#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Use.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include <vector>
#include <unordered_map>
#include <set>

using namespace llvm;

namespace {
  struct sumNode {
    Value* funcReturnVal = NULL;  //Return val propogation
    std::unordered_map<int, Value*> funcInputArgs; //CAT input Arg propogation
    std::unordered_map<int, Value*> ConstArgs; //NonCAT input Arg propogation
  };

  struct CAT : public ModulePass {
    static char ID; 
    Module *currM;
    const DataLayout *DL;
    CAT() : ModulePass(ID) {}
    Function* CAT_new;
    Function* CAT_set;
    std::unordered_map<Function*,sumNode > summary;
    std::vector<Instruction*> Insts;
    std::unordered_map<Instruction*, llvm::BitVector> genSet, killSet, inSet, outSet; 
    std::unordered_map<Instruction*, Value*> propogatedConstants;
    std::unordered_map<Instruction*, Value*> foldedConstants;
    std::unordered_map<Instruction*,  std::set<Instruction*>> mayMustAliases, mustAliases;
    std::set< CallInst* > nonCATCalls;
    std::vector< StoreInst* > storeInsts;
    std::vector< Instruction* > memInsts;
    std::set< StoreInst* > escapedStores; 
    AliasAnalysis *aliasAnalysis; 
    CallGraph *CG;



    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point 
    bool doInitialization (Module &M) override {
        currM = &M;
        DL = &(currM->getDataLayout());
        Constant* catN = M.getOrInsertFunction("CAT_new", FunctionType::get(PointerType::get(IntegerType::get(M.getContext(), 8), 0), IntegerType::get(M.getContext(), 64), false ));
        CAT_new = cast<Function>(catN);
        std::vector<Type*> argTypes;
        argTypes.push_back(PointerType::get(IntegerType::get(M.getContext(), 8), 0));
        argTypes.push_back(IntegerType::get(M.getContext(), 64));
        Constant* set = M.getOrInsertFunction("CAT_set", FunctionType::get(Type::getVoidTy(M.getContext()), argTypes, false ));
        CAT_set = cast<Function>(set);
        return false;
    }

    bool runOnModule(Module &M) override {

        CG = &(getAnalysis<CallGraphWrapperPass>().getCallGraph());

        bool modified = false;

        findInlinableFuncs(M);

        modified |= inlineFunctions(M); //Inline functions whenever safe

        modified |= cloneFunctions(M); //Clone the remaining function calls so that each calle has a single callsite to enable input Arg propogation

        if (modified) //Exit pass and recompile for updated LoopInfoWrapperPass, DominatorTreeWrapperPass, etc
            return true;

        modified |= transformLoops(M); //Loop unrolling and peeling for functions with < 500 IR instructions

        getSummary(M);

        modified |= transformFunctions(M); //constant folding and constant propogation passes

        return modified;
    }


    void findInlinableFuncs(Module &M){
        for(auto &F : M){
            CallGraphNode *n = (*CG)[&F];
            if(F.isDeclaration()) continue;
            if(F.getNumUses() == 0) continue; //Skip if function is never called
            if(n->size() > 100) continue; 
            if (isRecursiveFunc(&F, &F)) continue;
            F.setDoesNotRecurse();                  
        }
    }


    bool inlineFunctions(Module &M){
        bool modified = false;

        for(auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions
            modified = inlineCallees(F);
        }
        return modified;
    }

    bool inlineCallees(Function &F) {
        bool modified = false;
        bool inlined = false;

        for (auto &B : F) {
            for (auto &I : B) {
                if (auto callInst = dyn_cast<CallInst>(&I)){

                    auto calleeF = callInst->getCalledFunction();
                    if (callInst == nullptr) continue ; //Skip indirect calls
                    if(!calleeF->doesNotRecurse()) continue;



                    errs() << "\n Trying to Inline " << calleeF->getName() << " to " << F.getName()<<" at callsite ";
                    callInst->print(errs());

                    InlineFunctionInfo  IFI;
                    inlined |= InlineFunction(callInst, IFI);
                    if (inlined) {    
                        errs()<<" -- Succeeded";                
                        modified = true;
                        break ;
                    } 
                    else{
                        errs()<<"\t -- Failed to inline";
                    }
                }
            }    
        }

        if (inlined){
            inlineCallees(F); //Recursive inlining
        }

        return modified;
    }

    bool isRecursiveFunc(Function *F, Function *calleeF){
        bool recursive = false;
        std::vector<Function *> calledFuncs;
        for (auto &B : *calleeF) {
            for (auto &I : B) {
                if (auto call = dyn_cast<CallInst>(&I)){
                    if(call->getCalledFunction() == F){
                        errs()<<"\n FOUND recursive path for Function: "<<F->getName()<<"\n\n";
                        return true;
                    }
                    else{
                        if(call->getCalledFunction()->isDeclaration()) continue;
                        calledFuncs.push_back(call->getCalledFunction());
                    }
                }
            }
        }
        if(!calledFuncs.empty()){
            for(auto calledF : calledFuncs){
                if(isRecursiveFunc(F  , calledF));
                    return true;
            }
        }
        return recursive;
    }

    bool cloneFunctions(Module &M){
        bool modified = false;
        for(auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions
            modified = cloneCallees(F); //clone all callees until each callee has a single use/callsite
        }
        return modified;
    }


    bool cloneCallees(Function &F) {
        bool modified = false;
            
        for (auto &B : F) {
            for (auto &I : B) {
                if (auto callInst = dyn_cast<CallInst>(&I)){
                    auto calleeF = callInst->getCalledFunction();
                    if (callInst == nullptr) continue ; //Skip indirect calls
                    if(!calleeF->doesNotRecurse()) continue;
                    if (calleeF->getNumUses()<2) continue; //Skip if If numUses of callee is 1



                    errs() << "Cloning " << calleeF->getName() << " from " << F.getName() << "\n";
                    ValueToValueMapTy VMap;
                    auto clonedCallee = CloneFunction(calleeF, VMap);
                    callInst->replaceUsesOfWith(calleeF, clonedCallee);
                    modified = true;    
                }
            }                  
        }

        return modified;
    }

    bool transformLoops(Module &M){
        bool modified = false;
        for(auto &F : M){
            if(F.isDeclaration()) continue;
            if((F.getNumUses() == 0) && (F.getName() != "main")) continue; //Skip if function is never called  
            if(F.getInstructionCount() > 500)  continue;       
            auto& LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
            auto& DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
            auto& SE = getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
            auto &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F); 
            OptimizationRemarkEmitter ORE(&F);
            errs() << "\n TransformLoops pass for Function: " << F.getName() << "\n";
            for (auto i : LI){
                auto loop = &*i;
                modified |= analyzeLoop(LI, loop, DT, SE, AC, ORE);
            }
        }
        return modified;
    }


    bool analyzeLoop (
        LoopInfo &LI, 
        Loop *loop, 
        DominatorTree &DT, 
        ScalarEvolution &SE, 
        AssumptionCache &AC,
        OptimizationRemarkEmitter &ORE
        ){

        // if (!loop->isLCSSAForm(DT)){
        //     formLCSSARecursively(*loop, DT, &LI, nullptr);
        // }

        std::vector<Loop *> subLoops = loop->getSubLoops();

        /* 
        * Analyze sub-loops of the loop given as input.
        */

        for (auto j : subLoops){
            auto subloop = &*j;
            if (analyzeLoop(LI, subloop, DT, SE, AC, ORE)){
                return true;
            }
        }

        if(loop->getLoopDepth() >3) //skip nested loops with depth >3
            return false;


        /*
        * Check if the loop has been normalized.
        */

        if (loop->isLoopSimplifyForm()  && loop->isLCSSAForm(DT)){

            /*
             * Identify the trip count.
             */
            auto tripCount = SE.getSmallConstantTripCount(loop);
            auto tripMultiple = SE.getSmallConstantTripMultiple(loop);
            errs() << "\n   Trip count: " << tripCount << "\n";
            errs() << "\n   Trip multiple: " << tripMultiple << "\n";

            /*
             * Try to unroll the loop
             */
            auto forceUnroll = false;
            auto allowRuntime = true;
            auto allowExpensiveTripCount = true;
            auto preserveCondBr = false;
            auto preserveOnlyFirst = false;
            auto peelingCount = 1;
            auto unrollFactor = 2;
            LoopUnrollResult unrolled;

            if ((tripCount >0) && (tripCount<20)){
                unrollFactor = tripCount;
                peelingCount = 0;
            }

              unrolled = UnrollLoop(
              loop, unrollFactor, 
              tripCount, 
              forceUnroll, allowRuntime, allowExpensiveTripCount, preserveCondBr, preserveOnlyFirst, 
              tripMultiple, peelingCount, 
              false, 
              &LI, &SE, &DT, &AC, &ORE,
               true);

            if (unrolled != LoopUnrollResult::Unmodified ){
              return true ;
            }            
        }

        return false;
    }
    

    void getSummary(Module &M){
        int i = 0;
        while(i++<2){
            for (auto &F : M){
                if(F.isDeclaration()) continue; //Skip externally declared functions  
                if(F.getNumUses() == 0) continue; //Skip if function is never called
                errs()<<"\n\n getSummary for :"<<F.getName();       
                CATFuncAnalyse(F); //Initializes sets and computes aliases and reaching defs         
                getRetConstant(F); //If a function returns a constant value, add it to summary funcReturnVals[F] to enable retVal propogation at callsite;    
                getCalleeArgConstants(F);  
            }
        }
    }


    bool transformFunctions(Module &M){
        bool modified = false;
        for (auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions 
            if((F.getNumUses() == 0) && (F.getName() != "main")) continue; //Skip if function is never called  
            errs()<<"\n\nCAT_Transform Pass for :"<<F.getName();      
            CATFuncAnalyse(F); //Initializes sets and computes aliases and reaching defs   
            modified |= ConstArgPropogation(F); //nonCAT inter-procedural constant propogation
            modified |= CATFuncTransform(F); //Constant propogation and folding pass    
        }   
        return modified;
    }

    void getRetConstant(Function &F){
        errs()<<"\nchecking for returnVal Constant from "<<F.getName();
        for(auto &bb : F){
            for(auto &i : bb){                         
                if(auto *retInst = dyn_cast<ReturnInst>(&i)){
                    if(retInst->getReturnValue() == NULL) continue;
                    Value *constant_to_propogate = NULL;
                    constant_to_propogate = getConstant(&i, 0, false);
                    if(constant_to_propogate != NULL){
                        summary[&F].funcReturnVal = constant_to_propogate;                                    
                    }
                    return;
                }
            }
        }
    }

    void getCalleeArgConstants(Function &F){ 
        for(auto call : nonCATCalls){  
        	if (call->getCalledFunction()->getNumUses()>1) continue; //Skip if If numUses of callee is more than 1
            //if(call->getCalledFunction()->isDeclaration()) continue; //Skip externally declared functions     
            if(call->getCalledFunction() == &F) continue; //Skip recursive functions
            for (int i = 0; i < call->getNumArgOperands(); i++) {

                if(auto constInt = dyn_cast<ConstantInt>(call->getArgOperand(i))){
                    summary[call->getCalledFunction()].ConstArgs[i] = constInt;
                    errs()<<"\nFound a NONCAT constant \""<<constInt->getSExtValue()<<"\" for input argument \""<<i;
                    errs()<<"\" of callee: "<<call->getCalledFunction()->getName();
                    errs()<<"\n";
                }

                Value *constant_to_propogate = NULL;   
                bool escapedstore = false;            
                for(auto store : storeInsts){
                    if(call->getArgOperand(i) == store->getPointerOperand()){
                        escapedstore = true;
                        constant_to_propogate = getConstant(store, 0, true);
                        break;
                    }
                }
                if(!escapedstore){
                    if(auto *defInst = dyn_cast<Instruction>(call->getArgOperand(i))){
                        constant_to_propogate = getConstant(call, i, true);
                    }
                }
                
                if(constant_to_propogate != NULL){
                    summary[call->getCalledFunction()].funcInputArgs[i] = constant_to_propogate;
                    ConstantInt* const1 = dyn_cast<ConstantInt>(constant_to_propogate);
                    errs()<<"\nFound a constant \""<<const1->getSExtValue()<<"\" for input argument \""<<i;
                    errs()<<"\" of callee: "<<call->getCalledFunction()->getName();
                    errs()<<"\n";
                }              
            }            
        }
    }

    bool ConstArgPropogation(Function &F){
        bool modified = false;
        std::vector<Argument*> constArgs;
        if(summary.find(&F) != summary.end()){
            for(auto &arg : F.args()) {
                int argNum = arg.getArgNo();
                if(summary[&F].ConstArgs.find(argNum) != summary[&F].ConstArgs.end()){
                    if(summary[&F].ConstArgs[argNum] != NULL){
                        constArgs.push_back(&arg); 
                        modified = true;   
                    }
                }
            }
        }


        for(auto arg : constArgs){
            int argNum = arg->getArgNo();
            Value* constInt = summary[&F].ConstArgs[argNum];
            // for (auto& U : arg->uses()) {
            //     U.getUser()->setOperand(U.getOperandNo(), constInt);  
            // }
            arg->replaceAllUsesWith(constInt);
        }
        return modified;
    }

    bool CATFuncTransform(Function &F) {


        bool modified = false;
        
        bool propogated = constantPropogation();    

        bool folded = constantFolding();

        if(propogated){
            replacePropogatedConstants();
            modified = true;                
        }

        if(folded){
            replaceFoldedConstants();
            modified = true;
        }
        

        modified |= deadCodeElimination(F) | constantFoldnonCAT(F);

        return modified;
    }



    void CATFuncAnalyse(Function &F){

        aliasAnalysis = &(getAnalysis< AAResultsWrapperPass >(F).getAAResults());
        Insts.clear();
        genSet.clear();
        killSet.clear();
        inSet.clear();
        outSet.clear();
        nonCATCalls.clear();
        memInsts.clear();
        storeInsts.clear();
        propogatedConstants.clear();
        foldedConstants.clear();
        mayMustAliases.clear();
        mustAliases.clear();
        for(auto &bb : F){
            for(auto &i : bb){
                Insts.push_back(&i);
                if(auto *call = dyn_cast<CallInst>(&i)){
                    if( (call->getCalledFunction()->getName() != "CAT_new") &&
                        (call->getCalledFunction()->getName() != "CAT_add") &&
                        (call->getCalledFunction()->getName() != "CAT_sub") &&
                        (call->getCalledFunction()->getName() != "CAT_set") &&
                        (call->getCalledFunction()->getName() != "CAT_get")){
                        if(!(call->getCalledFunction()->isDeclaration()))
                            nonCATCalls.insert(call);
                        //errs()<<"\n nonCATcall: \t";
                        //  i.print(errs());
                                          
                    }
                }
                else if ( isa<LoadInst>(&i)){
                    memInsts.push_back(&i);
                    // errs()<<"\n load: \t";
                    //  I->print(errs());
                }
                else if (auto store = dyn_cast<StoreInst>(&i)){
                    storeInsts.push_back(store);
                    memInsts.push_back(&i);
                    // errs()<<"\n store: \t";
                    //  I->print(errs());
                }                                                                    
            }
        } 

        for (int i = 0; i < Insts.size(); i++){
            genSet[Insts[i]] = BitVector(Insts.size(), false);
            killSet[Insts[i]] = BitVector(Insts.size(), false); 
            inSet[Insts[i]] = BitVector(Insts.size(), false);
            outSet[Insts[i]] = BitVector(Insts.size(), false);
        }

        computeAliases(F);
        computeGenKill(F);
        computeInOut(F); 
        //printReachingDefSets(F);
        //printAliasSets(F);
    }

    void computeAliases(Function &F){

        for(auto memInst1 : memInsts){
            for (auto memInst2 : memInsts){
                if(memInst1 == memInst2) continue;
                switch (aliasAnalysis->alias(MemoryLocation::get(memInst1), MemoryLocation::get(memInst2))){
                    case MustAlias:
                    {

                        Instruction* Alias1 = memInst1;
                        Instruction* Alias2 = memInst2;
                        if(StoreInst* store1 = dyn_cast<StoreInst>(memInst1)){
                            if(!(Alias1 = dyn_cast<Instruction>(store1->getValueOperand()))) break;
                            if ( isa<BitCastInst>(Alias1)) break;
                        }
                        
                        if(StoreInst* store2 = dyn_cast<StoreInst>(memInst2)){

                            if(!(Alias2 = dyn_cast<Instruction>(store2->getValueOperand()))) break;
                            if ( isa<BitCastInst>(Alias2)) break;
                        }
                        
                        mustAliases[Alias1].insert(Alias2);
                        mustAliases[Alias2].insert(Alias1);
                        mayMustAliases[Alias1].insert(Alias2);
                        mayMustAliases[Alias2].insert(Alias1); 
                        break;
                    }
                    case MayAlias: case PartialAlias:
                    {

                        Instruction* Alias1 = memInst1;
                        Instruction* Alias2 = memInst2;
                        if(StoreInst* store1 = dyn_cast<StoreInst>(memInst1)){                            
                            if(!(Alias1 = dyn_cast<Instruction>(store1->getValueOperand()))) break;
                            if ( isa<BitCastInst>(Alias1)) break;
                        }
                        
                        if(StoreInst* store2 = dyn_cast<StoreInst>(memInst2)){

                            if(!(Alias2 = dyn_cast<Instruction>(store2->getValueOperand()))) break;
                            if ( isa<BitCastInst>(Alias2)) break;
                        }
                        
                        mayMustAliases[Alias1].insert(Alias2);
                        mayMustAliases[Alias2].insert(Alias1); 
                        break;
                    }

                    default:
                        break;                
                }
            }
        }



        for(auto call : nonCATCalls){
            for (int i = 0; i < call->getNumArgOperands(); i++) {
                for(auto store : storeInsts){
                    if(call->getArgOperand(i) == store->getPointerOperand()){
                        switch(aliasAnalysis->getModRefInfo(call ,MemoryLocation::get(store))){
                            case ModRefInfo::Mod: 
                            case ModRefInfo::Ref: 
                            case ModRefInfo::ModRef: 
                            case ModRefInfo::MustMod: 
                            case ModRefInfo::MustRef: 
                            case ModRefInfo::MustModRef:
                                escapedStores.insert(store);
                                break;
                            default:
                                break;
                        }       
                    }                                 
                }  
            }
        }
    }

    void computeGenKill(Function &F){

        int instCount = 0;
        for(auto &bb : F){
            for(auto &i : bb){
                if(CallInst *callInst = dyn_cast<CallInst>(&i)){
                    if((callInst->getCalledFunction()->getName() == "CAT_add") || 
                        (callInst->getCalledFunction()->getName() == "CAT_sub") || 
                        (callInst->getCalledFunction()->getName() == "CAT_set")){
                        genSet[&i].set(instCount);
                        markKills(&i);
                    }

                    if(callInst->getCalledFunction()->getName() == "CAT_new"){
                        genSet[&i].set(instCount);
                        for(int idx =0; idx<Insts.size(); idx++ ){
                            if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                                    (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                                    (curInst->getCalledFunction()->getName() == "CAT_set")){
                                    if(auto *curOperand = dyn_cast<Instruction>(curInst->getArgOperand(0))){
                                        if(curOperand == &i){
                                            killSet[&i].set(idx);
                                        }
                                    }
                                }
                            }
                            else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){
                                for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                                    if(curInst->getIncomingValue(inc) == &i){
                                        killSet[&i].set(idx);
                                    }
                                }   
                            }
                        }
                        markAliasedKills(&i, &i); // Kill[i] = Kill[i] U MustAlias[i]->redefinitions 
                    }                   
                }
                else if(PHINode *phiInst = dyn_cast<PHINode>(&i)){
                    genSet[&i].set(instCount);
                    for(int idx =0; idx<Insts.size(); idx++ ){
                        if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){                            
                            if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                                (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                                (curInst->getCalledFunction()->getName() == "CAT_set")){                                    
                                if(curInst->getArgOperand(0) == &i){
                                    killSet[&i].set(idx);
                                }                                    
                            }
                        }
                        else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){
                            for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                                if(curInst->getIncomingValue(inc) == &i){
                                    killSet[&i].set(idx);
                                }
                            }   
                        }
                    }
                    markAliasedKills(&i, &i); // Kill[i] = Kill[i] U MustAlias[i]->redefinitions    
                }

                killSet[&i].reset(instCount);
                instCount++;
            }
        }
    }

    void markKills(Instruction* i){

        Instruction* defInst;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(!(defInst = dyn_cast<Instruction>(callInst->getArgOperand(0)))) return;
                
        }

        for(int idx =0; idx<Insts.size(); idx++ ){
            if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                if(curInst->getCalledFunction()->getName() == "CAT_new"){                                   
                    if(defInst == Insts[idx]){
                        killSet[i].set(idx);
                    }                                          
                }
                if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                    (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                    (curInst->getCalledFunction()->getName() == "CAT_set")){                                    
                    if(curInst->getArgOperand(0) == defInst){
                        killSet[i].set(idx);
                    }                                    
                }
            }
            else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){
                for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                    if(curInst->getIncomingValue(inc) == defInst){
                        killSet[i].set(idx);
                    }
                }   
            }
        }

       markAliasedKills(i, defInst); // Kill[i] = Kill[i] U MustAlias[i]->redefinitions                    
        
    }

    void markAliasedKills(Instruction* i, Instruction* defInst){

        //  mark all 'must' aliases of I and their redefinitions (through CAT_adds, PHINodes, etc) in the KILL set of I

        if (mustAliases.find(defInst) != mustAliases.end()){
            for (auto aliasInst : mustAliases[defInst]){
                for(int idx =0; idx<Insts.size(); idx++ ){
                    if(Insts[idx] == aliasInst){
                        killSet[i].set(idx); 
                    }
                    if(auto *curInst = dyn_cast<CallInst>(Insts[idx])){
                        if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                            (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                            (curInst->getCalledFunction()->getName() == "CAT_set")){                                                                        
                            if(curInst->getArgOperand(0) == aliasInst){
                               killSet[i].set(idx);
                            }
                        } 
                    }
                    else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){                                
                        for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                            if(curInst->getIncomingValue(inc) == aliasInst){
                                killSet[i].set(idx);
                            }
                        }   
                    }
                }    
            }
        }        
    }

    void computeInOut(Function &F){

        std::unordered_map<Instruction *, std::set<Instruction*>> predecs;
        for(auto &bb : F){
            for (BasicBlock *PredBB : llvm::predecessors(&bb)){
                if(!predecs.count(&bb.front()))
                    predecs[&bb.front()] = std::set<Instruction*>();
                predecs[&bb.front()].insert(PredBB->getTerminator());
            }
        }

        bool flag = true;
        while(flag){
            flag = false;
            for (int i = 0; i < Insts.size(); i++){
                auto oldIn = inSet[Insts[i]];
                auto oldOut = outSet[Insts[i]];

                if(predecs.count(Insts[i])){
                    for(auto &pred: predecs[Insts[i]]){
                        inSet[Insts[i]] |= outSet[pred];
                    }
                }
                else if(i>0){
                    inSet[Insts[i]] |= outSet[Insts[i-1]];
                }
                llvm::BitVector currentOut = killSet[Insts[i]];
                currentOut.flip();
                currentOut &= inSet[Insts[i]];
                currentOut |= genSet[Insts[i]];
                outSet[Insts[i]] |= currentOut;

                if((outSet[Insts[i]] != oldOut ) || (inSet[Insts[i]] != oldIn ))
                    flag = true;
            }
        }    
    }


    bool constantPropogation(){

        bool modified = false;
        for (auto i : Insts){
            if(CallInst *callInst = dyn_cast<CallInst>(i)){
                Value *constant_to_propogate = NULL;
                if(callInst->getCalledFunction()->getName() == "CAT_get"){
                    constant_to_propogate = getConstant(i, 0, false);
                    if(constant_to_propogate != NULL){
                        propogatedConstants[i] = constant_to_propogate;
                        modified = true;
                    }
                }
            }
        }
        return modified;
    }


    bool constantFolding(){

        bool modified = false;
        
        for (auto i : Insts){

            if(CallInst *callInst = dyn_cast<CallInst>(i)){
                Value *constant_to_propogate = NULL;
                if((callInst->getCalledFunction()->getName() == "CAT_add") || (callInst->getCalledFunction()->getName() == "CAT_sub")){             
                    Value *op1 = getConstant(i, 1, false);
                    Value *op2 = getConstant(i, 2, false);
      
                    if((op1 != NULL) && (op2 != NULL)){
                        ConstantInt* const1 = dyn_cast<ConstantInt>(op1);
                        ConstantInt* const2 = dyn_cast<ConstantInt>(op2);

                        ConstantInt* result = NULL;
                        if(callInst->getCalledFunction()->getName() == "CAT_add")
                            result = ConstantInt::get(const1->getType(),const1->getSExtValue() + const2->getSExtValue());
                        else
                            result = ConstantInt::get(const1->getType(),const1->getSExtValue() - const2->getSExtValue());
                        foldedConstants[i] = result;
                        modified = true;
                    }                       
                }
            }
        }
        return modified;
    }

    Value* getConstant(Instruction* i, int op, bool isfuncArg){ //Check for constant propogation possibility for a CAT_get instruction i
        std::vector<ConstantInt *> constants;
        Instruction* defVar;
        Argument* inpArg;
        bool foundDefVar = false;
        bool foundArgConstant = false;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(callInst->getArgOperand(op)))){
                if(inpArg = dyn_cast<Argument>(callInst->getArgOperand(op))){
                    foundArgConstant = true;
                }
                else{
                    return NULL;    
                }
            }
            else{
                foundDefVar = true;
            }          
        }
        else if(auto *retInst = dyn_cast<ReturnInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(retInst->getReturnValue()))){
                if(inpArg = dyn_cast<Argument>(retInst->getReturnValue())){
                    foundArgConstant = true;    
                }
                else{
                    return NULL;    
                }
            }
            else{
                // errs()<<"\n\n";
                // defVar->print(errs());
                foundDefVar = true;
            } 
        }
        else if(auto *storeInst = dyn_cast<StoreInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(storeInst->getValueOperand()))){
                if(inpArg = dyn_cast<Argument>(storeInst->getValueOperand())){
                    foundArgConstant = true;  
                }
                else{
                    return NULL;    
                }
            } 
            else{
                foundDefVar = true;
            } 
        }

        if(foundDefVar){
            
            
            if(auto *loadInst = dyn_cast<LoadInst>(defVar)){
                if(inpArg = dyn_cast<Argument>(loadInst->getPointerOperand())){
                    foundArgConstant = true;     
                }
            }
        }


        ConstantInt* argConst;
        ConstantInt* calleeConst;
        bool getArgConst = false;
        bool getCalleeConst = false;


        if(foundArgConstant){
            int argNum = inpArg->getArgNo();
            if(summary.find(inpArg->getParent()) != summary.end()){
                if(summary[inpArg->getParent()].funcInputArgs.find(argNum) != summary[inpArg->getParent()].funcInputArgs.end()){
                    if(summary[inpArg->getParent()].funcInputArgs[argNum] != NULL){
                        ConstantInt* const1 = dyn_cast<ConstantInt>(summary[inpArg->getParent()].funcInputArgs[argNum]);
                        errs()<<"\nFound an input Arg constant: "<<const1->getSExtValue()<<" for function: "<<inpArg->getParent()->getName();
                        errs()<<" reaching instruction: ";
                        i->print(errs());
                        argConst = const1;
                        getArgConst = true;
                    }
                }
            }

            for(auto in : inSet[i].set_bits()){
                if(CallInst *callInst_in = dyn_cast<CallInst>(Insts[in])){
                    if((callInst_in->getCalledFunction()->getName() == "CAT_add") || 
                        (callInst_in->getCalledFunction()->getName() == "CAT_sub") ||
                        (callInst_in->getCalledFunction()->getName() == "CAT_set")){

                        if(callInst_in->getArgOperand(0) == inpArg){
                            getArgConst = false;                                             
                        }
                    }
                }
            }
        }

        if(foundDefVar){
            if(CallInst *defCall = dyn_cast<CallInst>(defVar)){            
                if(summary.find(defCall->getCalledFunction()) != summary.end()){
                    if(summary[defCall->getCalledFunction()].funcReturnVal != NULL){
                        ConstantInt* const1 = dyn_cast<ConstantInt>(summary[defCall->getCalledFunction()].funcReturnVal);
                        errs()<<"\nFound a constant: "<<const1->getSExtValue()<<" to propogate from callee: "<<defCall->getCalledFunction()->getName();
                        errs()<<" for instruction: ";
                        i->print(errs());
                        calleeConst = const1;
                        getCalleeConst = true;
                    }
                }
            }
        }


        if(foundDefVar){
            if (mayMustAliases.find(defVar) != mayMustAliases.end()){
                for (auto aliasInst : mayMustAliases[defVar]){
                    if (!decideToPropogate(i, op, aliasInst, constants, isfuncArg )){
                        constants.clear();
                        break;
                    }
                }
            }
            else{
                if (!decideToPropogate(i, op, NULL, constants, isfuncArg )){
                    constants.clear();
                }
            }
        }

        Value* constant_to_propogate = NULL;
        if(getArgConst) constants.push_back(argConst);
        if(getCalleeConst) constants.push_back(calleeConst);
        if(!constants.empty()){
            for(auto constant : constants){
                if(constant_to_propogate == NULL){
                    constant_to_propogate = constant;
                }
                else if(((ConstantInt *)constant_to_propogate)->getSExtValue() != constant->getSExtValue()){
                    return NULL;
                }
            }
        }  
        return constant_to_propogate;
    
    }

    bool decideToPropogate(Instruction* i, int op, Instruction* aliasInst, std::vector<ConstantInt *> &constants, bool isfuncArg){


        Instruction* defVar;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(callInst->getArgOperand(op)))) return false;
                
        }
        else if(auto *retInst = dyn_cast<ReturnInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(retInst->getReturnValue()))) return false;
        }
        else if(auto *storeInst = dyn_cast<StoreInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(storeInst->getValueOperand()))) return false;
        }

        bool defEscapes, aliasInstEscapes;
        if(!isfuncArg){
            defEscapes = isEscapedVar(defVar);
            aliasInstEscapes = isEscapedVar(aliasInst);
        }
        else{
            defEscapes = false;
            aliasInstEscapes = false;
        }

        for(auto in : inSet[i].set_bits()){
            if(CallInst *callInst_in = dyn_cast<CallInst>(Insts[in])){
                if(callInst_in->getCalledFunction()->getName() == "CAT_new"){
                    if( ((Insts[in] == defVar) && (!defEscapes)) || ((Insts[in] == aliasInst) && (!aliasInstEscapes)) ){   
                        if(isa<ConstantInt>(callInst_in->getArgOperand(0))){
                            constants.push_back((ConstantInt*)callInst_in->getArgOperand(0));
                        }
                        else{
                            return false;
                        }                       
                    }                   
                }
                else if(callInst_in->getCalledFunction()->getName() == "CAT_set"){
                    if(auto *setVar = dyn_cast<Instruction>(callInst_in->getArgOperand(0))){
                        if(  ((setVar == defVar) && (!defEscapes)) || ((setVar == aliasInst) && (!aliasInstEscapes))){                             
                            if(isa<ConstantInt>(callInst_in->getArgOperand(1))){
                                constants.push_back((ConstantInt*)callInst_in->getArgOperand(1));
                            }
                            else{
                                return false;
                            }                           
                        }                       
                    }
                }
                else if((callInst_in->getCalledFunction()->getName() == "CAT_add") || (callInst_in->getCalledFunction()->getName() == "CAT_sub")){
                    if(auto *setVar = dyn_cast<Instruction>(callInst_in->getArgOperand(0))){
                        if((setVar == defVar) || (setVar == aliasInst)){
                            return false;
                        }                       
                    }
                }
            }
            else if(PHINode *curInst = dyn_cast<PHINode>(Insts[in])){               
                ConstantInt* PHIvalue = NULL;
                if( ((Insts[in] == defVar) && (!defEscapes)) || ((Insts[in] == aliasInst) && (!aliasInstEscapes))){          
                    for(int index = 0; index < curInst->getNumIncomingValues(); index++){
                        if(auto *incVar = dyn_cast<CallInst>(curInst->getIncomingValue(index))){
                            if(incVar->getCalledFunction()->getName()== "CAT_new"){
                                if(auto *constantVar = dyn_cast<ConstantInt>(incVar->getArgOperand(0))){
                                    if(PHIvalue == NULL){
                                        PHIvalue = constantVar;
                                    }
                                    else if(PHIvalue->getSExtValue() != constantVar->getSExtValue()){
                                        PHIvalue = NULL;
                                        break;
                                    }
                                }
                            }
                        }
                        else{
                            return false;
                        }
                    }
                    if(PHIvalue != NULL){
                        constants.push_back(PHIvalue);
                    }
                    else{
                        return false;
                    }                    
                }                                                                       
            }
        }
        return true;
    }

    bool isEscapedVar(Instruction* defVar){ //Check if the variable escapes(ModRef) the function  since this is a conservative intra-procedural pass

        if (defVar == NULL) return false;
        // errs()<<"\n isEscapedVar check for: \t";
        // defVar->print(errs());
        bool varEscapes = false;
        for(auto &U  : defVar->uses()){
            // errs()<<"\n use: \t";
            // U.getUser()->print(errs());
            // errs()<<"\n";
            if(auto *useVar = dyn_cast<CallInst>(U.getUser())){
                Function* useFunc = useVar->getCalledFunction();
                if((useFunc->getName() != "CAT_new") &&
                    (useFunc->getName() != "CAT_add") &&
                    (useFunc->getName() != "CAT_sub") &&
                    (useFunc->getName() != "CAT_set") &&
                    (useFunc->getName() != "CAT_get")){
                    bool inputPropogated = false;
                    if(summary.find(useFunc) != summary.end()){
                        if(summary[useFunc].funcInputArgs.find(U.getOperandNo()) != summary[useVar->getCalledFunction()].funcInputArgs.end()){
                            if(summary[useFunc].funcInputArgs[U.getOperandNo()] != NULL){
                                for(auto &arg : useFunc->args()) {
                                    if(arg.getArgNo() == U.getOperandNo()){
                                        if(arg.getNumUses() == 0){
                                            inputPropogated = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if(!inputPropogated){
                        auto sizePointer = getPointedElementTypeSize(defVar);
                        switch(aliasAnalysis->getModRefInfo(useVar ,defVar, sizePointer)){
                            case ModRefInfo::Mod: 
                            case ModRefInfo::Ref: 
                            case ModRefInfo::ModRef: 
                            case ModRefInfo::MustMod: 
                            case ModRefInfo::MustRef: 
                            case ModRefInfo::MustModRef:
                                // errs()<<"\n\t defVar escapes\n";
                                return true;
                            default:
                                break;
                        }
                    }                   
                }                                   
            }
            else if(auto *storeInst = dyn_cast<StoreInst>(U.getUser())){

                if(escapedStores.find(storeInst) != escapedStores.end()) {
                    // errs()<<"\nFound an Escaped Store for use: \t";
                    // U.getUser()->print(errs());
                    return true;                    
                }
                else{
                    // errs()<<"\nChecking for Escaped Stores recursively for use: \t" ;
                    // U.getUser()->print(errs());
                    varEscapes = checkEscapedStores(storeInst); 
                }
            }
        }
        return varEscapes;
    }

    uint64_t getPointedElementTypeSize (Instruction *pointer){
        uint64_t size = 0;

        if (auto pointerType = dyn_cast<PointerType>(pointer->getType())){
          auto elementPointedType = pointerType->getElementType();
          if (elementPointedType->isSized()){
            size = currM->getDataLayout().getTypeStoreSize(elementPointedType);
          }
        }

        return size;
    }

    bool checkEscapedStores(StoreInst* defStore){ //recursively check for escaped stores
        for(auto i : Insts){
            if(auto *storeInst = dyn_cast<StoreInst>(i)){
                if(storeInst->getValueOperand() == defStore->getPointerOperand()){
                    if(escapedStores.find(storeInst) != escapedStores.end()) {
                        // errs()<<"\nFound an Escaped Store through recursive check: \t" ;
                        // storeInst->print(errs());
                        return true;                    
                    }
                    else{
                        return checkEscapedStores(storeInst);   
                    }
                }
            }
        }
        return false;
    }

    void replacePropogatedConstants(){  //Replace all uses of propogated Constants
        for(auto &i : propogatedConstants){
            ConstantInt* const1 = dyn_cast<ConstantInt>(i.second);
            errs()<<"\nReplaced all uses of instruction: "; 
            i.first->print(errs());  
            errs()<<" with value "<<const1->getSExtValue()<<" by Constant Propogation";               
            BasicBlock::iterator ii(i.first);
            ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);                    
        }
    }

    void replaceFoldedConstants(){ //Repalce all folded instructions with CAT_set

        for(auto &i : foldedConstants){
            IRBuilder<>builder(i.first);
            std::vector<Value*> args;
            CallInst *CAT_operation = dyn_cast<CallInst>(i.first);
            errs()<<"\nFolding instruction: "; 
            CAT_operation->print(errs()); 
            args.push_back((CAT_operation->getArgOperand(0)));
            args.push_back(i.second);
            Instruction* CatSet =  cast<Instruction>(builder.CreateCall(CAT_set, ArrayRef<Value *>(args)));
            errs()<<"\t with ";
            CatSet->print(errs());
        }
        for(auto &i : foldedConstants){
            i.first->eraseFromParent();
        }
    }

    bool deadCodeElimination(Function &F){ //Path-insensitive DCE pass

        bool flag = true;
        bool modified = true;
        std::set<Instruction *> toDelete;

        while(flag){
            flag = false;
            for(auto &bb : F){
                for(auto &i : bb){
                    if(CallInst *callInst = dyn_cast<CallInst>(&i)){
                        if((callInst->getCalledFunction()->getName() != "CAT_new") &&
                            (callInst->getCalledFunction()->getName() != "CAT_add") &&
                            (callInst->getCalledFunction()->getName() != "CAT_sub") &&
                            (callInst->getCalledFunction()->getName() != "CAT_set") &&
                            (callInst->getCalledFunction()->getName() != "CAT_get")){

                            if (isDeadCall(F, callInst)){
                                toDelete.insert(callInst);
                                continue;
                            }
                        }
                        else if(callInst->getCalledFunction()->getName() == "CAT_new"){
                            if(callInst->getNumUses() == 0){
                                toDelete.insert(callInst);
                                continue;
                            }
                            else{
                                std::set<Instruction *> tempDelete;
                                for (auto &U : callInst->uses()) {
                                    User* user = U.getUser();
                                    if (auto *useInst = dyn_cast<CallInst>(user)){      
                                        if((useInst->getCalledFunction()->getName() == "CAT_add") || 
                                            (useInst->getCalledFunction()->getName() == "CAT_sub") || 
                                            (useInst->getCalledFunction()->getName() == "CAT_set")){
                                            if(useInst->getArgOperand(0) != callInst){
                                                tempDelete.clear();
                                                break;
                                            }

                                            bool aliasDependence = false;
                                            if (mayMustAliases.find(callInst) != mayMustAliases.end()){
                                                for (auto aliasInst : mayMustAliases[callInst]){

                                                    for (auto &AliasUser : aliasInst->uses()) {
                                                        if (auto *AliasUseInst = dyn_cast<CallInst>(AliasUser.getUser())){  
                                                            if(AliasUseInst->getCalledFunction()->getName() == "CAT_get"){
                                                                aliasDependence = true;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            if(aliasDependence){
                                                tempDelete.clear();
                                                break;
                                            }

                                            tempDelete.insert(useInst);
                                        }
                                        if(useInst->getCalledFunction()->getName() == "CAT_get"){
                                            if(useInst->getArgOperand(0) == callInst){
                                                tempDelete.clear();
                                                break;
                                            }
                                        }
                                    }     
                                }
                                toDelete.insert(tempDelete.begin(), tempDelete.end());
                            }
                        }
                    }
                    else if(PHINode *PHIInst = dyn_cast<PHINode>(&i)){
                       if(PHIInst->getNumUses() == 0){
                            toDelete.insert(PHIInst);
                            continue;
                        }   
                    }
                }
            }


            for (auto I : toDelete) {
                errs()<<"\n Deleted instruction: ";
                I->print(errs());
                errs()<<"  by DCE";
               
            }
            for (auto I : toDelete) {
                I->eraseFromParent();
                flag = true;
                modified = true;
            }
            toDelete.clear();
        }

        return modified;
    }

    bool isDeadCall(Function &F, CallInst* callInst){
        bool escapedCall = false;
        Function* calleeF = callInst->getCalledFunction();
        if (calleeF == &F) return false; //skip recursive calls
        if(calleeF->isDeclaration()){
            if((calleeF->getName() == "CAT_new") ||
                (calleeF->getName() == "CAT_get")){
                return true;
            }
            else{
                return false;
            }                
        }

        else if(callInst->getNumUses() == 0){
            for(auto &bb : *calleeF){
                for(auto &i : bb){
                    if(StoreInst *newStore = dyn_cast<StoreInst>(&i)){
                        if (isa<GlobalValue>(newStore->getPointerOperand())){
                            return false;
                            break;
                        }
                    }
                    if(CallInst *newCall = dyn_cast<CallInst>(&i)){
                       if(!isDeadCall(*calleeF, newCall)){
                            return false;
                            break;
                        }
                    }

                    for (auto &U : calleeF->uses()){
                        auto user = U.getUser();
                        if (auto recCallInst = dyn_cast<Instruction>(user)){
                            if(recCallInst->getFunction() == calleeF){
                               return false;
                               break; 
                            }
                        }
                    }
                	
                }
            }
            return true;
        }
        else{
            return false;
        }
        
    }



    bool constantFoldnonCAT(Function &F){ //There is atleast one test (test1) for which this pass helps reduce #CAT_invocations

        std::unordered_map<Instruction*, Value*> llvmFoldedConstants;
        bool modified = false;
        bool flag = true;
        while(flag){
            flag = false;
            llvmFoldedConstants.clear();
            for(auto &bb : F){
                for(auto &i : bb){
                    Constant* const1 = ConstantFoldInstruction(&i, *DL);
                    if(const1 != NULL){                  
                        llvmFoldedConstants[&i] = const1;
                        continue;                                 
                    }
                }
            }
            for(auto &i : llvmFoldedConstants){
                errs()<<"\n Replaced all uses of instruction: "; 
                i.first->print(errs());     
                errs()<<" using ConstantFoldInstruction() library function";            
                BasicBlock::iterator ii(i.first);
                ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);
                modified = true;
                flag = true;                   
            }
        }  
        return modified;   
    }

    void printReachingDefSets(Function &F){
        errs() << "START FUNCTION: " << F.getName() << '\n';
        for (int i = 0; i < Insts.size(); i++){               
            errs()<<"INSTRUCTION: ";  
            Insts[i]->print(errs());

            if(genSet[Insts[i]].any()){
                errs()<<"\n***************** GEN\n{\n";
                printOutputSet(genSet[Insts[i]], Insts);
            }

            if(killSet[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** KILL\n{\n";
                printOutputSet(killSet[Insts[i]], Insts);
            }

            if(inSet[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** IN\n{\n";            
                printOutputSet(inSet[Insts[i]], Insts);
            }

            if(outSet[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** OUT\n{\n";
                printOutputSet(outSet[Insts[i]], Insts);
            }

            errs() << "}\n**************************************\n\n\n\n";                                                            
        }
    }


    void printOutputSet(BitVector &set, std::vector<Instruction *> Insts){
        for(auto in : set.set_bits()){
            errs()<<" ";
            Insts[in]->print(errs());
            errs()<<"\n";                        
        }
    }

    void printAliasSets(Function &F){
        errs()<<"\n\n\n Must Aliases for "<<F.getName()<<": \n";
        printAliases(mustAliases);
        errs()<<"\n\n\n May and Must Aliases for "<<F.getName()<<": \n";
        printAliases(mayMustAliases);
    }

    void printAliases(std::unordered_map<Instruction *,  std::set<Instruction*>> &Aliases){
        for (auto I : Insts){
            if (Aliases.find(I) == Aliases.end()) continue;
            errs()<<"\n\n Alias for : \n";
            I->print(errs());
            errs()<<"\n Aliases:";
            for (auto aliasInst : Aliases[I]){
                errs()<<"\n";
                aliasInst->print(errs());
            }          
        }
    }

    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired< CallGraphWrapperPass >();
      AU.addRequired< AAResultsWrapperPass >();
      AU.addRequired<AssumptionCacheTracker>();
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.addRequired<LoopInfoWrapperPass>();
      AU.addRequired<ScalarEvolutionWrapperPass>();
      //AU.setPreservesAll();
    }
  };
}


// Next there is code to register your pass to "opt"
char CAT::ID = 0;
static RegisterPass<CAT> X("CAT", "Homework for the CAT class");

// Next there is code to register your pass to "clang"
static CAT * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT()); }}); // ** for -O0

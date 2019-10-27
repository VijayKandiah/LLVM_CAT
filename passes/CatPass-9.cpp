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
#include "llvm/Transforms/Utils/LoopRotationUtils.h"
#include "llvm/Analysis/InstructionSimplify.h"


#include <vector>
#include <unordered_map>
#include <set>

using namespace llvm;

namespace {

  struct FunctionSummary {
    std::vector<Instruction*> Insts;
    std::vector<BasicBlock*> CATbbs;
    std::unordered_map<BasicBlock*, std::vector<Instruction* >> CATInsts; //CAT insts for BB
    std::unordered_map<Instruction*, unsigned> indexMap;
    Value* funcReturnVal = NULL;  //Return val propogation
    std::unordered_map<int, Value*> funcInputArgs; //CAT input Arg propogation
    std::unordered_map<int, Value*> ConstArgs; //NonCAT input Arg propogation
    std::unordered_map<Instruction*, llvm::BitVector> genInst, killInst, inInst, outInst;
    std::unordered_map<BasicBlock*, llvm::BitVector> genBB, killBB, inBB, outBB;  
    std::unordered_map<Instruction*, Value*> propogatedConstants;
    std::unordered_map<Instruction*, Value*> foldedConstants;
    std::unordered_map<Instruction* , Instruction*> getReplaceMap;
    std::set<Instruction*> CATSetsToDelete;
    std::unordered_map<Instruction*,  std::set<Instruction*>> mayMustAliases, mustAliases;
    std::set< CallInst* > nonCATCalls;
    std::vector< StoreInst* > storeInsts;
    std::vector< Instruction* > memInsts;
    std::set< StoreInst* > escapedStores; 
    AliasAnalysis  *aliasAnalysis;    
  };


  struct CAT : public ModulePass {
    static char ID; 
    Module *currM;
    const DataLayout *DL;
    CAT() : ModulePass(ID) {}
    Function* CAT_new;
    Function* CAT_set;
    Function* CAT_add;
    Function* CAT_sub;
    Function* CAT_get;
    Function* mainF;
    std::unordered_map<Function*,FunctionSummary* > summaryNode;
    CallGraph *CG;


    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point 
    bool doInitialization (Module &M) override {

        currM = &M;
        DL = &(currM->getDataLayout());
        // Constant* catN = M.getOrInsertFunction("CAT_new", FunctionType::get(PointerType::get(IntegerType::get(M.getContext(), 8), 0), IntegerType::get(M.getContext(), 64), false ));
        //CAT_new = cast<Function>(catN);
        CAT_new = M.getFunction("CAT_new");
        CAT_add = M.getFunction("CAT_add");
        CAT_sub = M.getFunction("CAT_sub");
        CAT_get = M.getFunction("CAT_get");
        mainF = M.getFunction("main");
        std::vector<Type*> argTypes;
        argTypes.push_back(PointerType::get(IntegerType::get(M.getContext(), 8), 0));
        argTypes.push_back(IntegerType::get(M.getContext(), 64));
        Constant* set = M.getOrInsertFunction("CAT_set", FunctionType::get(Type::getVoidTy(M.getContext()), argTypes, false ));
        CAT_set = cast<Function>(set); //Create a Func signature for CAT_set since it's used for constant folding and Module may not have it declared
        for(auto &F : M){
            summaryNode[&F] = new FunctionSummary();
        }
        return false;
    }


    bool runOnModule(Module &M) override {

        CG = &(getAnalysis<CallGraphWrapperPass>().getCallGraph());

        bool modified = false;

        findInlinableFuncs(M); //check for direct or indirect function recursions

        modified |= inlineFunctions(M); //Inline functions whenever safe

        modified |= cloneFunctions(M); //Clone the remaining function calls so that each calle has a single callsite to enable input Arg propogation

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
            if(n->size() > 100) continue; //Ignore functions which have 100+ Calls
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

                    //errs() << "\n Trying to Inline " << calleeF->getName() << " to " << F.getName()<<" at callsite ";
                    //callInst->print(errs());

                    InlineFunctionInfo  IFI;
                    inlined |= InlineFunction(callInst, IFI);
                    if (inlined) {    
                        //errs()<<" -- Succeeded";                
                        modified = true;
                        break ;
                    } 
                    else{
                        //errs()<<"\t -- Failed to inline";
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
                        //errs()<<"\n FOUND recursive path for Function: "<<F->getName()<<"\n";
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
                    //errs() << "Cloning " << calleeF->getName() << " from " << F.getName() << "\n";
                    ValueToValueMapTy VMap;
                    auto clonedCallee = CloneFunction(calleeF, VMap);
                    callInst->replaceUsesOfWith(calleeF, clonedCallee);
                    modified = true;    
                }
            }                  
        }

        return modified;
    }


    bool hasCATCalls(Loop *loop){
        for (auto bbi = loop->block_begin(); bbi != loop->block_end(); ++bbi){
            BasicBlock *bb = *bbi;
            for (auto &i : *bb) {
                if (auto callInst = dyn_cast<CallInst>(&i)){
                    Function* calleeF = callInst->getCalledFunction();
                    if((calleeF == CAT_new) ||
                        (calleeF == CAT_get) ||
                        (calleeF == CAT_add) ||
                        (calleeF == CAT_sub) ||
                        (calleeF == CAT_set)){
                        return true;
                    }
                }
            }
        }
        return false;
    }



    bool transformLoops(Module &M){

        bool modified = false;
        
        for(auto &F : M){
            if(F.isDeclaration()) continue;
            if((F.getNumUses() == 0) && (&F != mainF)) continue; //Skip if function is never called
            if(F.getInstructionCount() > 500) continue;    //don't unroll loops for functions with over 500 IR instructions  
            auto& LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
            auto& DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
            auto& SE = getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
            auto &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F); 
            auto &TTI = getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
            OptimizationRemarkEmitter ORE(&F);
              //errs() << "\n\n TransformLoops pass for Function: " << F.getName() << " with "<<F.getInstructionCount()<<" instructions\n";  
            std::vector<Loop* > toPeel;
            for (auto i : LI){
                auto loop = &*i;
                if(!hasCATCalls(loop)) continue;   
                toPeel.push_back(loop);
            }
            for (auto i: toPeel){
                modified |= unrollLoop(LI, i, DT, SE, AC, ORE, TTI);
            }
            modified |= deleteCondBrs(F);          
        }
        return modified;
    }


    bool unrollLoop (
        LoopInfo &LI, 
        Loop *loop, 
        DominatorTree &DT, 
        ScalarEvolution &SE, 
        AssumptionCache &AC,
        OptimizationRemarkEmitter &ORE,
        TargetTransformInfo &TTI
        ){

        bool modified = false;

        std::vector<Loop *> subLoops = loop->getSubLoops();
        for (auto j : subLoops){
            auto subloop = &*j;
            if(subloop->getLoopDepth() >3) continue;
            if (unrollLoop(LI, subloop, DT, SE, AC, ORE, TTI)){
                return true;
            }       
        }
        
        if(loop->getLoopDepth() >3) //skip nested loops with depth >3
            return false;

        //errs()<<"\n\nTrying to unroll/peel loop at depth "<<loop->getLoopDepth();
       
        if (!(loop->isLoopSimplifyForm()  && loop->isLCSSAForm(DT))){
            //errs()<<"\nloop not normalized"; 
            return false; 
        }
           
        //errs()<<"\n\nTrying to unroll loop";
        auto tripCount = SE.getSmallConstantTripCount(loop);
        auto tripMultiple = SE.getSmallConstantTripMultiple(loop);
        auto forceUnroll = false;
        auto allowRuntime = true;
        auto allowExpensiveTripCount = true;
        auto preserveCondBr = false;
        auto preserveOnlyFirst = false;
        auto peelingCount = 1;
        auto unrollFactor = 2;
        LoopUnrollResult unrolled;


        if(tripMultiple > 1){
            if ((tripCount >0) && (tripCount<20)){
                unrollFactor = tripCount;
                peelingCount = 0;
            }
             // errs() << "\n   Trip count: " << tripCount << "\n";
             // errs() << "\n   Trip multiple: " << tripMultiple << "\n";
            unrolled = UnrollLoop(
                loop, unrollFactor, 
                tripCount, 
                forceUnroll, allowRuntime, allowExpensiveTripCount, preserveCondBr, preserveOnlyFirst, 
                tripMultiple, peelingCount, 
                false, 
                &LI, &SE, &DT, &AC, &ORE,
                true);

            if (unrolled != LoopUnrollResult::Unmodified ){
                //errs()<<"\nloop unrolling-- success";
                return true;
            }
            else{
                //errs()<<"\nloop unrolling -- failed";

            }
        }
        // if(loop->getLoopDepth() >1) return false;
        //errs()<<"\n\nTrying to peel loop";
        if(!canPeel(loop)){
            BasicBlock *LatchBlock = loop->getLoopLatch();
            BranchInst *BI = dyn_cast<BranchInst>(LatchBlock->getTerminator());
            if (!BI || BI->isUnconditional()) {
                SimplifyQuery SQ(*DL, BI);
                bool rotated = LoopRotation(loop, 
                    &LI, &TTI, &AC, &DT, &SE, 
                    nullptr, SQ, 
                    false, -1, false); //rotate loop to make it peelable
                //if (canPeel(loop))
                //errs()<<"\nRotated loop for peeling/unrolling";
            }
        }
        if(!canPeel(loop)) return false;
        peelingCount = 10;
        //errs() << "\n   PeelingCount: " << peelingCount << "\n";
        if(peelLoop( loop, peelingCount, &LI, &SE, &DT, &AC, true)){
            //errs() << "   Peeled\n";
            return true ;
        }
        else{
            //errs()<<"peeling failed\n";
        }
        
               
        return false;
    }


    bool deleteCondBrs(Function &F){
        std::unordered_map<Instruction*,  unsigned> condBranches;
        for(auto &bb : F){
            for(auto &i : bb){
                if(auto branchInst = dyn_cast<BranchInst>(&i)){
                    if(branchInst->isUnconditional()) continue;
                    if(auto constInt = dyn_cast<ConstantInt>(branchInst->getCondition())){
                        bool branchTarget = false;
                        if(constInt->isZero())
                            branchTarget = true;
                        condBranches[branchInst] = branchTarget;
                        // errs()<<"\nchaning branch :";
                        // branchInst->print(errs());
                        // errs()<<"\nremoving successor "<<!branchTarget;
                        BasicBlock* succToRemove = branchInst->getSuccessor(!branchTarget);
                        succToRemove->removePredecessor(&bb);
                        
                    }
                } 
            }
        }

        if(condBranches.empty()) return false;
        
        for(auto &i : condBranches){
            IRBuilder<>builder(i.first);
            BranchInst *branch = dyn_cast<BranchInst>(i.first);
            // errs()<<"\nChanging cond branch "; 
            // branch->print(errs()); 
            Instruction* uncondBranch =  cast<Instruction>(builder.CreateBr(branch->getSuccessor(i.second)));
            // errs()<<"\t to Uncond: ";
            // uncondBranch->print(errs());
        }
        for(auto &i : condBranches){
            i.first->eraseFromParent();
        }
        bool changed = true;
        while(changed){
            changed = false;
            std::set<BasicBlock*> toDeleteBBs;
            for(auto &bb : F){
                if(&(F.getEntryBlock()) == &bb) continue;
                if(bb.hasNPredecessors(0) ){
                    //bb.replaceSuccessorsPhiUsesWith(&(F.getEntryBlock()));
                    for (int i = 0; i< bb.getTerminator()->getNumSuccessors(); i++){
                        BasicBlock* PredBB = bb.getTerminator()->getSuccessor(i);
                        PredBB->removePredecessor(&bb);
                        for(auto &i : *PredBB){
                            if(auto phi = dyn_cast<PHINode>(&i)){  
                                if(phi->getBasicBlockIndex(&bb) < 0) continue;
                                phi->removeIncomingValue(&bb, true);
                            }
                        }
                    }
                }
            }
            for(auto &bb : F){
                if(&(F.getEntryBlock()) == &bb) continue;
                if(bb.hasNPredecessors(0) ){
                    toDeleteBBs.insert(&bb);
                    changed = true;
                }
            }

            for(auto bb : toDeleteBBs){
                bb->eraseFromParent();
                changed = true;
            }
        }
        return true;
    }
    

    void getSummary(Module &M){

        for (auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions         
            CATFuncAnalyse(F); //Initializes sets and computes aliases and reaching defs       
        } 

        int i = 0;
        while(i++<2){
            for (auto &F : M){
                if(F.isDeclaration()) continue; //Skip externally declared functions      
                getRetConstant(F); //If a function returns a constant value, add it to summaryNode funcReturnVals[F] to enable retVal propogation at callsite;    
                getCalleeArgConstants(F);  

            }
        }
    }


    void getRetConstant(Function &F){

        if(F.getNumUses() == 0) return;
        if(F.getReturnType()->isVoidTy()) return;
        //errs()<<"\nchecking for returnVal Constant from "<<F.getName();
        for(auto &bb : F){
            for(auto &i : bb){                         
                if(auto *retInst = dyn_cast<ReturnInst>(&i)){
                    Value *constant_to_propogate = NULL;
                    constant_to_propogate = getConstant(&i, 0, false);
                    if(constant_to_propogate != NULL){
                        //errs()<<"\nFound a returnVal constant \""<<((ConstantInt*)constant_to_propogate)->getSExtValue()<<"\" for function \""<<F.getName()<<"\n";
                        summaryNode[&F]->funcReturnVal = constant_to_propogate;                                    
                    }
                    return;
                }
            }
        }
    }


    void getCalleeArgConstants(Function &F){ 

        for(auto call : summaryNode[&F]->nonCATCalls){  
            Function* calleeF = call->getCalledFunction();
            if(calleeF->isDeclaration()) continue; //Skip externally declared functions  
            if(calleeF->getNumUses()>1) continue; //Skip if If numUses of callee is more than 1    
            if(calleeF == &F) continue; //Skip recursive functions
            for (int i = 0; i < call->getNumArgOperands(); i++) {
                //errs()<<"\nChecking for a constant for input argument \""<<i<<"\" of callee: "<<call->getCalledFunction()->getName()<<"\n";
                auto* arg = call->getArgOperand(i);
                if(auto constInt = dyn_cast<ConstantInt>(arg)){
                    summaryNode[call->getCalledFunction()]->ConstArgs[i] = constInt;
                    // errs()<<"\nFound a NONCAT constant \""<<constInt->getSExtValue()<<"\" for input argument \""<<i;
                    // errs()<<"\" of callee: "<<call->getCalledFunction()->getName()<<"\n";
                }
                else{
                    Value *constant_to_propogate = NULL;   
                    bool escapedstore = false;            
                    for(auto store : summaryNode[&F]->storeInsts){
                        if(arg == store->getPointerOperand()){
                            escapedstore = true;
                            constant_to_propogate = getConstant(store, 0, true);
                            break;
                        }
                    }
                    if(!escapedstore){
                        if(auto *defInst = dyn_cast<Instruction>(arg)){
                            constant_to_propogate = getConstant(call, i, true);
                        }
                    }
                    
                    if(constant_to_propogate != NULL){
                        summaryNode[call->getCalledFunction()]->funcInputArgs[i] = constant_to_propogate;
                        ConstantInt* const1 = dyn_cast<ConstantInt>(constant_to_propogate);
                        // errs()<<"\nFound a CAT constant \""<<const1->getSExtValue()<<"\" for input argument \""<<i;
                        // errs()<<"\" of callee: "<<call->getCalledFunction()->getName()<<"\n";
                    }
                }              
            }            
        }
    }


    bool transformFunctions(Module &M){

        bool modified = false;
        for (auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions 
            if((F.getNumUses() == 0) && (&F != mainF)) continue; //Skip if function is never called  
            //errs()<<"\n\nCAT_Transform Pass for :"<<F.getName();      
            modified |= ConstArgPropogation(F); //nonCAT inter-procedural constant propogation done first
            modified |= CATFuncTransform(F); //Constant propogation and folding pass    
        }   
        return modified;
    }


    bool ConstArgPropogation(Function &F){

        bool modified = false;
        std::vector<Argument*> constArgs;
        if(summaryNode.find(&F) != summaryNode.end()){
            for(auto &arg : F.args()) {
                int argNum = arg.getArgNo();
                if(summaryNode[&F]->ConstArgs.find(argNum) != summaryNode[&F]->ConstArgs.end()){
                    if(summaryNode[&F]->ConstArgs[argNum] != NULL){
                        constArgs.push_back(&arg); 
                        modified = true;   
                    }
                }
            }
        }

        for(auto arg : constArgs){
            int argNum = arg->getArgNo();
            Value* constInt = summaryNode[&F]->ConstArgs[argNum];
            arg->replaceAllUsesWith(constInt);
        }
        return modified;
    }


    bool CATFuncTransform(Function &F) {

        bool modified = false;

        bool propogated = constantPropogation(F);    

        bool folded = constantFolding(F);

        bool copied = CATGetPropogation(F);

        if(propogated){
            replacePropogatedConstants(F);
            modified = true;                
        }

        if(copied){
            replaceCopiedConstants(F);
            modified = true;
        }


        if(folded){
            replaceFoldedConstants(F);
            modified = true;
        }

        modified |= deadCodeElimination(F) | constantFoldnonCAT(F);   


        return modified;
    }  


    void CATFuncAnalyse(Function &F){
        //errs()<<"\n\nCATFuncAnalyse for :"<<F.getName();
        FunctionSummary* sumF = summaryNode[&F];
        sumF->aliasAnalysis = &(getAnalysis< AAResultsWrapperPass >(F).getAAResults());
        bool isCATbb = false;
        unsigned instCount = 0;
        for(auto &bb : F){
            std::vector<Instruction* > currentCATInsts;
            for(auto &i : bb){
                sumF->Insts.push_back(&i);
                sumF->indexMap[&i] = instCount++;
                if(auto *call = dyn_cast<CallInst>(&i)){
                    Function* calleeF = call->getCalledFunction();
                    if( (calleeF == CAT_new) ||
                        (calleeF == CAT_add) ||
                        (calleeF == CAT_sub) ||
                        (calleeF == CAT_set) ||
                        (calleeF == CAT_get)){ 
                        currentCATInsts.push_back(&i);
                        isCATbb = true;
                        //errs()<<"\n nonCATcall: \t";
                        //  i.print(errs());                                         
                    }
                    else{
                        sumF->nonCATCalls.insert(call);
                        if(!calleeF->isDeclaration()){
                            currentCATInsts.push_back(&i);
                            isCATbb = true;
                        }
                    }
                }
                else if ( isa<LoadInst>(&i)){
                    sumF->memInsts.push_back(&i);
                    // errs()<<"\n load: \t";
                    //  I->print(errs());
                }
                else if (auto store = dyn_cast<StoreInst>(&i)){
                    sumF->storeInsts.push_back(store);
                    sumF->memInsts.push_back(&i);
                    // errs()<<"\n store: \t";
                    //  I->print(errs());
                }
                else if (auto phiInst = dyn_cast<PHINode>(&i)){   
                    if (auto* callPhi = dyn_cast<CallInst>(phiInst->getIncomingValue(0))) {
                        if(callPhi->getCalledFunction() == CAT_new){
                            currentCATInsts.push_back(&i);
                            isCATbb = true;
                        }
                    }
                    else if(isa<LoadInst>(phiInst->getIncomingValue(0))){
                        currentCATInsts.push_back(&i);
                        isCATbb = true;
                    }
                    else if(isa<PHINode>(phiInst->getIncomingValue(0))){
                        currentCATInsts.push_back(&i);
                        isCATbb = true;
                    }
                }                                                              
            }
            if(isCATbb){
                sumF->CATbbs.push_back(&bb);
                sumF->CATInsts[&bb] = currentCATInsts;
            }
        } 

        for(auto bb : sumF->CATbbs){
            for(auto i : sumF->CATInsts[bb]){
                sumF->genInst[i] = BitVector(sumF->Insts.size(), false);
                sumF->killInst[i] = BitVector(sumF->Insts.size(), false); 
                sumF->inInst[i] = BitVector(sumF->Insts.size(), false);
                sumF->outInst[i] = BitVector(sumF->Insts.size(), false);
            }
        }
        for(auto &bb : F){
            sumF->genBB[&bb] = BitVector(sumF->Insts.size(), false);
            sumF->killBB[&bb] = BitVector(sumF->Insts.size(), false);
            sumF->inBB[&bb] = BitVector(sumF->Insts.size(), false);
            sumF->outBB[&bb] = BitVector(sumF->Insts.size(), false);
        }
        computeAliases(F);
        computeGenKill(F);
        computeInOut(F); 
        //printReachingDefSets(F);
        // printAliasSets(F);
    }

    void computeAliases(Function &F){

        //errs()<<"\nComputing Alias sets for "<<F.getName();
        FunctionSummary* sumF = summaryNode[&F];
        for(auto memInst1 : sumF->memInsts){
            for (auto memInst2 : sumF->memInsts){
                if(memInst1 == memInst2) continue;
                switch (sumF->aliasAnalysis->alias(MemoryLocation::get(memInst1), MemoryLocation::get(memInst2))){
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
                        
                        sumF->mustAliases[Alias1].insert(Alias2);
                        sumF->mustAliases[Alias2].insert(Alias1);
                        sumF->mayMustAliases[Alias1].insert(Alias2);
                        sumF->mayMustAliases[Alias2].insert(Alias1); 
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
                        
                        sumF->mayMustAliases[Alias1].insert(Alias2);
                        sumF->mayMustAliases[Alias2].insert(Alias1); 
                        break;
                    }

                    default:
                        break;                
                }
            }
        }

        for(auto call : sumF->nonCATCalls){
            for (int i = 0; i < call->getNumArgOperands(); i++) {
                for(auto store : sumF->storeInsts){
                    if(call->getArgOperand(i) == store->getPointerOperand()){
                        switch(sumF->aliasAnalysis->getModRefInfo(call ,MemoryLocation::get(store))){
                            case ModRefInfo::Mod: 
                            case ModRefInfo::Ref: 
                            case ModRefInfo::ModRef: 
                            case ModRefInfo::MustMod: 
                            case ModRefInfo::MustRef: 
                            case ModRefInfo::MustModRef:
                                sumF->escapedStores.insert(store);
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
        //errs()<<"\nComputing GenKill sets for "<<F.getName();
        FunctionSummary* sumF = summaryNode[&F];
        for(auto bb : sumF->CATbbs){
            for(auto i : sumF->CATInsts[bb]){
                if(CallInst *callInst = dyn_cast<CallInst>(i)){
                    Function* calleeF = callInst->getCalledFunction();
                    if((calleeF == CAT_add) || 
                        (calleeF == CAT_sub) || 
                        (calleeF == CAT_set)){
                        sumF->genInst[i].set(sumF->indexMap[i]);
                        markKills(i);
                    }
                    else if(calleeF == CAT_new){
                        sumF->genInst[i].set(sumF->indexMap[i]);
                        markKills(i);
 
                    }                   
                }
                else if(PHINode *phiInst = dyn_cast<PHINode>(i)){
                    sumF->genInst[i].set(sumF->indexMap[i]);
                    markKills(i);
                    
                }
                sumF->killInst[i].reset(sumF->indexMap[i]);
            }
        }

        for(auto bb : sumF->CATbbs){
            for (auto i = sumF->CATInsts[bb].rbegin(); i != sumF->CATInsts[bb].rend(); ++i) { //reverse-traversal
                Instruction* inst = *i;
                llvm::BitVector currentGen = sumF->genBB[bb];
                llvm::BitVector currentKill = sumF->killBB[bb];
                currentGen.flip();
                currentKill.flip();
                currentKill &= sumF->genInst[inst];
                sumF->genBB[bb] |= currentKill;
                currentGen &= sumF->killInst[inst];
                sumF->killBB[bb] |= currentGen; 
            }
        }
    }


    void markKills(Instruction* i){
        Function* F = i->getFunction();
        FunctionSummary* sumF = summaryNode[F];
        Instruction* defInst;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(callInst->getCalledFunction() == CAT_new){
                defInst = callInst;
            }
            else if(!(defInst = dyn_cast<Instruction>(callInst->getArgOperand(0)))) return;  
        }
        else if(isa<PHINode>(i)){
            defInst = i;
        }
        else
            return;
        
        sumF->killInst[i].set(sumF->indexMap[defInst]);
        for(auto &U : defInst->uses()){
            User* user = U.getUser();
            if (auto *useInst = dyn_cast<CallInst>(user)){ 
                Function* calleeF = useInst->getCalledFunction(); 
                if((calleeF == CAT_add) || 
                    (calleeF == CAT_sub) || 
                    (calleeF == CAT_set)){
                    if(useInst->getArgOperand(0) == defInst){

                        sumF->killInst[i].set(sumF->indexMap[useInst]);
                    }
                }
            }
            else if (auto *useInst = dyn_cast<PHINode>(user)){ 
                sumF->killInst[i].set(sumF->indexMap[useInst]);
            }
        }

       markAliases(i, defInst); // Kill[i] = Kill[i] U MustAlias[i]->redefinitions                    
        
    }


    void markAliases(Instruction* i, Instruction* defInst){

        //  mark all 'must' aliases of I and their redefinitions (through CAT_adds, PHINodes, etc) in the KILL set of I
        Function* F = i->getFunction();
        FunctionSummary* sumF = summaryNode[F]; 

        if (sumF->mustAliases.find(defInst) != sumF->mustAliases.end()){
            for (auto aliasInst : sumF->mustAliases[defInst]){
                sumF->killInst[i].set(sumF->indexMap[aliasInst]);
                for(auto &U : aliasInst->uses()){
                    User* user = U.getUser();
                    if (auto *useInst = dyn_cast<CallInst>(user)){ 
                        Function* calleeF = useInst->getCalledFunction(); 
                        if((calleeF == CAT_add) || 
                                (calleeF == CAT_sub) || 
                                (calleeF == CAT_set)){
                            if(useInst->getArgOperand(0) == aliasInst){
                                sumF->killInst[i].set(sumF->indexMap[useInst]);
                            }
                        }
                    }
                    else if (auto *useInst = dyn_cast<PHINode>(user)){ 
                        sumF->killInst[i].set(sumF->indexMap[useInst]);
                    }
                }
            }
        }        
    }


    void computeInOut(Function &F){
        //errs()<<"\nComputing InOut sets for "<<F.getName();
        FunctionSummary* sumF = summaryNode[&F];
        std::vector<Instruction*> Insts = sumF->Insts;
        std::vector<BasicBlock*> BBs;
        std::unordered_map<BasicBlock *, std::set<BasicBlock*>> predecs;
        std::unordered_map<BasicBlock *, std::set<BasicBlock*>> succs;
        for(auto &bb : F){
            BBs.push_back(&bb);
            for (BasicBlock *PredBB : llvm::predecessors(&bb)){
                if(!predecs.count(&bb))
                    predecs[&bb] = std::set<BasicBlock*>();
                predecs[&bb].insert(PredBB);
            }
        }

        while(!BBs.empty()){
            BasicBlock* bb = BBs.front();
            BBs.erase(BBs.begin());

            for (auto pred : predecs[bb]) {
                sumF->inBB[bb] |= sumF->outBB[pred];
            }
            llvm::BitVector currentOut = sumF->killBB[bb];
            currentOut.flip();
            currentOut &= sumF->inBB[bb];
            currentOut |= sumF->genBB[bb];
            auto oldOut = sumF->outBB[bb];
            sumF->outBB[bb] |= currentOut;
            if(sumF->outBB[bb] != oldOut ){                
                BBs.push_back(bb);               
            }
        }

        for(auto bb : sumF->CATbbs){
            Instruction* prevI;
            for(auto i : sumF->CATInsts[bb]){
                if( i == sumF->CATInsts[bb].front()){
                    sumF->inInst[i] = sumF->inBB[bb];
                    llvm::BitVector currentOut = sumF->killInst[i];
                    currentOut.flip();
                    currentOut &= sumF->inInst[i];
                    currentOut |= sumF->genInst[i];
                    sumF->outInst[i] |= currentOut;
                    
                }
                else{
                    sumF->inInst[i] = sumF->outInst[prevI];
                    llvm::BitVector currentOut = sumF->killInst[i];
                    currentOut.flip();
                    currentOut &= sumF->inInst[i];
                    currentOut |= sumF->genInst[i];
                    sumF->outInst[i] |= currentOut;
                }
                prevI = i;

            }
        }           
    }


    bool constantPropogation(Function &F){
        FunctionSummary* sumF = summaryNode[&F];
        bool modified = false;
        for(auto bb : sumF->CATbbs){
            for(auto i : sumF->CATInsts[bb]){
                if(CallInst *callInst = dyn_cast<CallInst>(i)){
                    Value *constant_to_propogate = NULL;
                    if(callInst->getCalledFunction() == CAT_get){
                        if(isa<ConstantPointerNull>(callInst->getArgOperand(0))){
                            auto retType = dyn_cast<IntegerType>(callInst->getCalledFunction()->getReturnType());
                            sumF->propogatedConstants[i] = ConstantInt::get(retType,0, true);
                            modified = true;
                        }
                        else{
                            constant_to_propogate = getConstant(i, 0, false);
                            if(constant_to_propogate != NULL){
                                sumF->propogatedConstants[i] = constant_to_propogate;
                                modified = true;
                            }
                        }
                    }
                }
            }
        }
        return modified;
    }


    bool CATGetPropogation(Function &F){
        FunctionSummary* sumF = summaryNode[&F];
        auto& DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
        bool modified = false;
        for(auto bb : sumF->CATbbs){
            for(auto i : sumF->CATInsts[bb]){
                if(auto callInst = dyn_cast<CallInst>(i)){
                    if(callInst->getCalledFunction() != CAT_new) continue;
                    std::vector<CallInst*> catGETS;
                    for (auto &U : callInst->uses()) {
                        User* user = U.getUser();
                        if (auto *useInst = dyn_cast<CallInst>(user)){
                            Function* useCallee = useInst->getCalledFunction();  
                            if((useCallee != CAT_get)) continue;
                            catGETS.push_back(useInst);
                            
                        }
                    }
                    for(auto get1 : catGETS){
                        for(auto get2 : catGETS){
                            if(get1 == get2) continue;
                            if(DT.dominates(get1, get2)){
                                bool canReplace = true;
                                auto defVar = dyn_cast<Instruction>(get2->getArgOperand(0));
                                llvm::BitVector InSetDiff = sumF->inInst[get1];
                                InSetDiff.flip();
                                InSetDiff &= sumF->inInst[get2];
                                 for(auto in : InSetDiff.set_bits()){
                                    if(CallInst *callInst_in = dyn_cast<CallInst>(sumF->Insts[in])){
                                        Function* calleeF = callInst_in->getCalledFunction();
                                        if((calleeF == CAT_set) || (calleeF == CAT_add) || (calleeF == CAT_sub)){
                                            canReplace = false;
                                        }
                                        else if(!calleeF->isDeclaration()){
                                            canReplace = false;
                                        }
                                    }
                                }
                                if(canReplace){
                                    sumF->getReplaceMap[get2] = get1;
                                    modified = true;
                                }

                            }
                        }
                    }
                }
            }
        }


        return modified;

    }



    bool constantFolding(Function &F){
        FunctionSummary* sumF = summaryNode[&F];
        bool modified = false;
        for(auto bb : sumF->CATbbs){
            for(auto i : sumF->CATInsts[bb]){
                if(CallInst *callInst = dyn_cast<CallInst>(i)){
                    Function* calleeF = callInst->getCalledFunction();
                    if((calleeF == CAT_add) || (calleeF == CAT_sub)){             
                        Value *op1 = getConstant(i, 1, false);
                        Value *op2 = getConstant(i, 2, false);
          
                        if((op1 != NULL) && (op2 != NULL)){
                            ConstantInt* const1 = dyn_cast<ConstantInt>(op1);
                            ConstantInt* const2 = dyn_cast<ConstantInt>(op2);

                            ConstantInt* result = NULL;
                            if(calleeF == CAT_add)
                                result = ConstantInt::get(const1->getType(),const1->getSExtValue() + const2->getSExtValue());
                            else
                                result = ConstantInt::get(const1->getType(),const1->getSExtValue() - const2->getSExtValue());
                            sumF->foldedConstants[i] = result;
                            modified = true;
                        }                       
                    }
                    else if(calleeF == CAT_set){
                        Value *constant_to_propogate = NULL;
                        ConstantInt* constSet;
                        if(!(constSet = dyn_cast<ConstantInt>(callInst->getArgOperand(1)))) continue;
                        constant_to_propogate = getConstant(callInst, 0, false);
                        if(constant_to_propogate == NULL) continue;
                        ConstantInt* reachingConst = dyn_cast<ConstantInt>(constant_to_propogate);
                        if (reachingConst->getSExtValue() != constSet->getSExtValue()) continue;
                        sumF->CATSetsToDelete.insert(callInst);    
                        modified = true;                           
                    }
                }
            }
        }
        return modified;
    }


    Value* getConstant(Instruction* i, int op, bool isfuncArg){ //Check for constant propogation possibility for a CAT_get instruction i
        FunctionSummary* sumF = summaryNode[i->getFunction()];
        std::vector<ConstantInt *> constants;
        Instruction* defVar;
        Argument* inpArg;
        bool foundDefVar = false;
        bool foundArgVar = false;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(callInst->getArgOperand(op)))){
                if(inpArg = dyn_cast<Argument>(callInst->getArgOperand(op))){
                    foundArgVar = true;
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
                    foundArgVar = true;    
                }
                else{
                    return NULL;    
                }
            }
            else{
                foundDefVar = true;
            } 
        }
        else if(auto *storeInst = dyn_cast<StoreInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(storeInst->getValueOperand()))){
                if(inpArg = dyn_cast<Argument>(storeInst->getValueOperand())){
                    foundArgVar = true;  
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
                    foundArgVar = true;     
                }
            }
        }


        ConstantInt* argConst;
        ConstantInt* calleeConst;
        bool getArgConst = false;
        bool getCalleeConst = false;


        if(foundArgVar){
            int argNum = inpArg->getArgNo();
            if(summaryNode.find(inpArg->getParent()) != summaryNode.end()){
                if(summaryNode[inpArg->getParent()]->funcInputArgs.find(argNum) != summaryNode[inpArg->getParent()]->funcInputArgs.end()){
                    if(summaryNode[inpArg->getParent()]->funcInputArgs[argNum] != NULL){
                        ConstantInt* const1 = dyn_cast<ConstantInt>(summaryNode[inpArg->getParent()]->funcInputArgs[argNum]);
                        // errs()<<"\nFound an input Arg constant: "<<const1->getSExtValue()<<" for function: "<<inpArg->getParent()->getName();
                        // errs()<<" reaching instruction: ";
                        // i->print(errs());
                        argConst = const1;
                        getArgConst = true;
                    }
                }
            }

            for(auto in : sumF->inInst[i].set_bits()){
                if(CallInst *callInst_in = dyn_cast<CallInst>(sumF->Insts[in])){
                    Function* calleeF = callInst_in->getCalledFunction();
                    if((calleeF == CAT_add) || 
                        (calleeF == CAT_sub) ||
                        (calleeF == CAT_set)){

                        if(callInst_in->getArgOperand(0) == inpArg){
                            getArgConst = false;                                             
                        }
                    }
                }
            }
        }

        if(foundDefVar){
            if(CallInst *defCall = dyn_cast<CallInst>(defVar)){            
                if(summaryNode.find(defCall->getCalledFunction()) != summaryNode.end()){
                    if(summaryNode[defCall->getCalledFunction()]->funcReturnVal != NULL){
                        ConstantInt* const1 = dyn_cast<ConstantInt>(summaryNode[defCall->getCalledFunction()]->funcReturnVal);
                        // errs()<<"\nFound a constant: "<<const1->getSExtValue()<<" to propogate from callee: "<<defCall->getCalledFunction()->getName();
                        // errs()<<" for instruction: ";
                        // i->print(errs());
                        calleeConst = const1;
                        getCalleeConst = true;
                    }
                }
            }

            if (sumF->mayMustAliases.find(defVar) != sumF->mayMustAliases.end()){
                for (auto aliasInst : sumF->mayMustAliases[defVar]){
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

        FunctionSummary* sumF = summaryNode[i->getFunction()];
        std::vector<Instruction*> Insts = sumF->Insts;
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

        for(auto in : sumF->inInst[i].set_bits()){
            if(CallInst *callInst_in = dyn_cast<CallInst>(Insts[in])){
                Function* calleeF = callInst_in->getCalledFunction();
                if(calleeF == CAT_new){
                    if( ((Insts[in] == defVar) && (!defEscapes)) || ((Insts[in] == aliasInst) && (!aliasInstEscapes)) ){   
                        if(isa<ConstantInt>(callInst_in->getArgOperand(0))){
                            constants.push_back((ConstantInt*)callInst_in->getArgOperand(0));
                        }
                        else{
                            return false;
                        }                       
                    }                   
                }
                else if(calleeF == CAT_set){
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
                else if((calleeF == CAT_add) || (calleeF == CAT_sub)){
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
                            if(incVar->getCalledFunction() == CAT_new){
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
        FunctionSummary* sumF = summaryNode[defVar->getFunction()];
        // errs()<<"\n isEscapedVar check for: \t";
        // defVar->print(errs());
        bool varEscapes = false;
        for(auto &U  : defVar->uses()){
            // errs()<<"\n use: \t";
            // U.getUser()->print(errs());
            // errs()<<"\n";
            if(auto *useVar = dyn_cast<CallInst>(U.getUser())){
                Function* calleeF = useVar->getCalledFunction();
                if((calleeF != CAT_new) &&
                    (calleeF != CAT_add) &&
                    (calleeF != CAT_sub) &&
                    (calleeF != CAT_set) &&
                    (calleeF != CAT_get)){
                    bool inputPropogated = false;
                    if(summaryNode.find(calleeF) != summaryNode.end()){
                        if(summaryNode[calleeF]->funcInputArgs.find(U.getOperandNo()) != summaryNode[useVar->getCalledFunction()]->funcInputArgs.end()){
                            if(summaryNode[calleeF]->funcInputArgs[U.getOperandNo()] != NULL){
                                for(auto &arg : calleeF->args()) {
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
                        switch(sumF->aliasAnalysis->getModRefInfo(useVar ,defVar, sizePointer)){
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

                if(sumF->escapedStores.find(storeInst) != sumF->escapedStores.end()) {
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
        FunctionSummary* sumF = summaryNode[defStore->getFunction()];
        std::vector<StoreInst*> storeInsts = sumF->storeInsts;
        std::set< StoreInst* > escapedStores =  sumF->escapedStores;
        for(auto store : storeInsts){
            if(store->getValueOperand() == defStore->getPointerOperand()){
                if( escapedStores.find(store) !=  escapedStores.end()) {
                    // errs()<<"\nFound an Escaped Store through recursive check: \t" ;
                    // storeInst->print(errs());
                    return true;                    
                }
                else{
                    return checkEscapedStores(store);   
                }
            }
        }
        return false;
    }

    void replacePropogatedConstants(Function &F){  //Replace all uses of propogated Constants
        FunctionSummary* sumF = summaryNode[&F];
        for(auto &i : sumF->propogatedConstants){
            sumF->getReplaceMap.erase(i.first);
            ConstantInt* const1 = dyn_cast<ConstantInt>(i.second);
            // errs()<<"\nReplaced all uses of instruction: "; 
            // i.first->print(errs());  
            // errs()<<" with value "<<const1->getSExtValue()<<" by Constant Propogation";               
            BasicBlock::iterator ii(i.first);
            ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);                    
        }

    }

    void replaceCopiedConstants(Function &F){
        FunctionSummary* sumF = summaryNode[&F];
        for(auto get: sumF->getReplaceMap){
            // errs()<<"\nReplaced all uses of CAT_GET instruction: "; 
            // get.first->print(errs());  
            // errs()<<" with CAT_GET instruction ";
            // get.second->print(errs()); 
            // errs()<<" by Copy Propogation";               
            BasicBlock::iterator ii(get.first);
            ReplaceInstWithValue(get.first->getParent()->getInstList(), ii, get.second); 
        }
    }





    void replaceFoldedConstants(Function &F){ //Repalce all folded instructions with CAT_set
        FunctionSummary* sumF = summaryNode[&F];
        for(auto &i : sumF->foldedConstants){
            IRBuilder<>builder(i.first);
            std::vector<Value*> args;
            CallInst *CAT_operation = dyn_cast<CallInst>(i.first);
            //errs()<<"\nFolding instruction: "; 
            //CAT_operation->print(errs()); 
            args.push_back((CAT_operation->getArgOperand(0)));
            args.push_back(i.second);
            Instruction* CatSet =  cast<Instruction>(builder.CreateCall(CAT_set, ArrayRef<Value *>(args)));
            //errs()<<"\t with ";
            //CatSet->print(errs());
        }
        for(auto &i : sumF->foldedConstants){
            i.first->eraseFromParent();
        }
        for(auto &i : sumF->CATSetsToDelete){
            i->eraseFromParent();
        }
    }

    bool deadCodeElimination(Function &F){ //Path-insensitive DCE 

        bool flag = true;
        bool modified = true;
        std::set<Instruction *> toDelete;
        FunctionSummary* sumF = summaryNode[&F];
        while(flag){
            flag = false;
            for(auto &bb : F){
                for(auto &i : bb){
                    if(CallInst *callInst = dyn_cast<CallInst>(&i)){
                        Function* calleeF = callInst->getCalledFunction();
                        if((calleeF != CAT_new) &&
                            (calleeF != CAT_add) &&
                            (calleeF != CAT_sub) &&
                            (calleeF != CAT_set) &&
                            (calleeF != CAT_get)){

                            if (isDeadCall(F, callInst)){
                                toDelete.insert(callInst);
                                continue;
                            }
                        }
                        else if(calleeF == CAT_new){
                            if(callInst->getNumUses() == 0){
                                toDelete.insert(callInst);
                                continue;
                            }
                            else{
                                std::set<Instruction *> tempDelete;
                                for (auto &U : callInst->uses()) {
                                    User* user = U.getUser();
                                    if (auto *useInst = dyn_cast<CallInst>(user)){
                                        Function* useCallee = useInst->getCalledFunction();  
                                        if((useCallee == CAT_add) || 
                                            (useCallee == CAT_sub) || 
                                            (useCallee == CAT_set)){
                                            if(useInst->getArgOperand(0) != callInst){
                                                tempDelete.clear();
                                                break;
                                            }

                                            bool aliasDependence = false;
                                            if (sumF->mayMustAliases.find(callInst) != sumF->mayMustAliases.end()){
                                                for (auto aliasInst : sumF->mayMustAliases[callInst]){

                                                    for (auto &AliasUser : aliasInst->uses()) {
                                                        if (auto *AliasUseInst = dyn_cast<CallInst>(AliasUser.getUser())){  
                                                            if(AliasUseInst->getCalledFunction() == CAT_get){
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
                                        if(useCallee == CAT_get){
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


            // for (auto I : toDelete) {
            //     errs()<<"\n Deleted instruction: ";
            //     I->print(errs());
            //     errs()<<"  by DCE";
               
            // }
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
        Function* calleeF = callInst->getCalledFunction();
        FunctionSummary* sumF = summaryNode[calleeF];
        if (calleeF == &F) return false; //skip recursive calls
        if(calleeF->isDeclaration()) return false;
        if(callInst->getNumUses() != 0) return false;

        for(auto store : sumF->storeInsts){
            if (isa<GlobalValue>(store->getPointerOperand())) return false;
        }   

        for(auto call : sumF->nonCATCalls){
            if(!isDeadCall(*calleeF, call)) return false;
        }      
        
        for (auto &U : calleeF->uses()){
            auto user = U.getUser();
            if (auto recCallInst = dyn_cast<Instruction>(user)){
                if(recCallInst->getFunction() == calleeF) return false;                  
            }
        }

        return true; 
    }



    bool constantFoldnonCAT(Function &F){ 

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
                // errs()<<"\n Replaced all uses of instruction: "; 
                // i.first->print(errs());     
                // errs()<<" using ConstantFoldInstruction() Libcall";            
                BasicBlock::iterator ii(i.first);
                ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);
                modified = true;
                flag = true;                   
            }
        }  
        return modified;   
    }

    void printReachingDefSets(Function &F){
        FunctionSummary* sumF = summaryNode[&F];
        std::vector<Instruction*> Insts = sumF->Insts;
        errs() << "START FUNCTION: " << F.getName() << '\n';
        for (int i = 0; i < Insts.size(); i++){               
            errs()<<"INSTRUCTION: ";  
            Insts[i]->print(errs());

            if(sumF->genInst[Insts[i]].any()){
                errs()<<"\n***************** GEN\n{\n";
                printOutputSet(sumF->genInst[Insts[i]], Insts);
            }

            if(sumF->killInst[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** KILL\n{\n";
                printOutputSet(sumF->killInst[Insts[i]], Insts);
            }

            if(sumF->inInst[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** IN\n{\n";            
                printOutputSet(sumF->inInst[Insts[i]], Insts);
            }

            if(sumF->outInst[Insts[i]].any()){
                errs() << "}\n**************************************\n***************** OUT\n{\n";
                printOutputSet(sumF->outInst[Insts[i]], Insts);
            }

            errs() << "}\n**************************************\n\n\n\n";                                                            
        }

        // for(auto &bb : F){
        //     errs()<<"BB: ";  
        //     bb.print(errs());

        //     if(sumF->genBB[&bb].any()){
        //         errs()<<"\n***************** GEN\n{\n";
        //         printOutputSet(sumF->genBB[&bb], Insts);
        //     }

        //     if(sumF->killBB[&bb].any()){
        //         errs() << "}\n**************************************\n***************** KILL\n{\n";
        //         printOutputSet(sumF->killBB[&bb], Insts);
        //     }

        //     if(sumF->inBB[&bb].any()){
        //         errs() << "}\n**************************************\n***************** IN\n{\n";            
        //         printOutputSet(sumF->inBB[&bb], Insts);
        //     }

        //     if(sumF->outBB[&bb].any()){
        //         errs() << "}\n**************************************\n***************** OUT\n{\n";
        //         printOutputSet(sumF->outBB[&bb], Insts);
        //     }

        //     errs() << "}\n**************************************\n\n\n\n";

        // }
    }


    void printOutputSet(BitVector &set, std::vector<Instruction *> Insts){
        for(auto in : set.set_bits()){
            errs()<<" ";
            Insts[in]->print(errs());
            errs()<<"\n";                        
        }
    }

    void printAliasSets(Function &F){
        errs()<<"\n\n Must Aliases for "<<F.getName()<<": \n";
        printAliases(summaryNode[&F]->mustAliases, summaryNode[&F]->Insts);
        errs()<<"\n\n May and Must Aliases for "<<F.getName()<<": \n";
        printAliases(summaryNode[&F]->mayMustAliases, summaryNode[&F]->Insts);
    }

    void printAliases(std::unordered_map<Instruction *,  std::set<Instruction*>> &Aliases, std::vector<Instruction *> Insts){
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
      AU.addRequired<TargetTransformInfoWrapperPass>();
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

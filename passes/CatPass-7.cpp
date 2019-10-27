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
#include <vector>
#include <unordered_map>
#include <set>

using namespace llvm;

namespace {
  struct sumNode {
    Value* funcReturnVal = NULL;  //Return val propogation
    //std::unordered_map<int, Value*> funcInputArgs; //input Arg propogation disabled for now
  };

  struct CAT : public ModulePass {
    static char ID; 
    Module *currM;
    CAT() : ModulePass(ID) {}
    std::unordered_map<Function*,sumNode > summary;
    std::vector<Instruction*> Insts;
    std::unordered_map<Instruction*, llvm::BitVector> genSet, killSet, inSet, outSet; 
    std::unordered_map<Instruction*, Value*> propogatedConstants;
    std::unordered_map<Instruction*, Value*> foldedConstants;
    AliasAnalysis *aliasAnalysis;
    std::unordered_map<Instruction*,  std::set<Instruction*>> mayMustAliases, mustAliases;
    std::set< CallInst* > nonCATCalls;
    std::vector< StoreInst* > storeInsts;
    std::vector< Instruction* > memInsts;
    std::set< StoreInst* > escapedStores;  
    CallGraph *CG;
    const DataLayout *DL;
    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point 
    bool doInitialization (Module &M) override {
        currM = &M;
        DL = &(currM->getDataLayout());
        return false;
    }

    bool runOnModule(Module &M) override {

        CG = &(getAnalysis<CallGraphWrapperPass>().getCallGraph());
        bool modified = false;

        modified |= inlineFunctions(M); //Inline functions whenever safe

        //modified |= cloneFunctions(M); //Clone the remaining function calls so that each calle has a single callsite to enable input Arg propogation -- disabled for now...
        
        getSummary(M);

        modified |= transformFunctions(M); //constant folding and constant propogation passes

        return modified;
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
        CallGraphNode *n = (*CG)[&F];

        for (auto callee : *n) {
            auto calleeNode = callee.second;
            auto callInst = callee.first;

            if (callInst == nullptr) continue ; //Skip indirect calls

            bool isRecursiveCall = false;
            for (auto &U : F.uses()){
                auto user = U.getUser();
                if (auto callInst = dyn_cast<Instruction>(user)){
                    if(callInst->getFunction() == &F){
                       isRecursiveCall = true;
                       break; 
                    }
                }
            }
            if (isRecursiveCall) continue; //Skip recursive calls

            if (auto call = dyn_cast<CallInst>(callInst)) {
   				
   				errs() << "Inlining " << call->getCalledFunction()->getName() << " to " << F.getName()<<" at callsite ";
   				call->print(errs());
                errs()<< "\n";
                InlineFunctionInfo  IFI;
                inlined |= InlineFunction(call, IFI);
                if (inlined) {                    
                    modified = true;
                    break ;
                } 
                else{
                	errs()<<"Failed to inline";
                }
            }
        }


        if (inlined){
            inlineCallees(F); //Recursive inlining
        }

        return modified;
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
                if (auto callInst = dyn_cast<CallInst>(&I)) {
                    auto calleeF = callInst->getCalledFunction();
                    if (calleeF->getNumUses()<2) continue; //Skip if If numUses of callee is one
                    if (callInst == nullptr) continue ; //Skip indirect calls
                    if(calleeF->isDeclaration()) continue; //Skip externally declared functions

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

    void getSummary(Module &M){
        for (auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions         
            CATFuncAnalyse(F); //Initializes sets and computes aliases and reaching defs         
            getRetConstant(F); //If a function returns a constant value, add it to summary funcReturnVals[F] to enable retVal propogation at callsite;    
        }
    }


    bool transformFunctions(Module &M){
    	bool modified = false;
        for (auto &F : M){
            if(F.isDeclaration()) continue; //Skip externally declared functions         
            CATFuncAnalyse(F); //Initializes sets and computes aliases and reaching defs         
            modified |= CATFuncTransform(F); //Constant propogation and folding pass    
        }   
        return modified;
    }

    void getRetConstant(Function &F){
    	if(F.getName() == "main") return; //Skip main 
        
        for(auto &bb : F){
            for(auto &i : bb){                         
                if(auto *retInst = dyn_cast<ReturnInst>(&i)){
                    if(retInst->getReturnValue() == NULL) continue;
                    Value *constant_to_propogate = NULL;
                    constant_to_propogate = getConstant(&i,  0);
                    if(constant_to_propogate != NULL){
                        summary[&F].funcReturnVal = constant_to_propogate;              
                    }
                    return;
                }
            }
        }
    }

    bool CATFuncTransform(Function &F) {

        aliasAnalysis = &(getAnalysis< AAResultsWrapperPass >(F).getAAResults());

        bool modified = false;
        
        bool propogated = constantPropogation();    

        bool folded = constantFolding();

        if(propogated){
            replacePropogatedConstants();
            modified = true;                
        }

        if(folded){
            folded = replaceFoldedConstants();
            modified |= folded;
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
        else if(auto *retInst = dyn_cast<ReturnInst>(i)){
            if(!(defInst = dyn_cast<Instruction>(retInst->getReturnValue()))) return;
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
                    constant_to_propogate = getConstant(i,  0);
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
                    Value *op1 = getConstant(i, 1);
                    Value *op2 = getConstant(i, 2);
      
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

    Value* getConstant(Instruction* i, int op){ //Check for constant propogation possibility for a CAT_get instruction i
        std::vector<ConstantInt *> constants;
        Instruction* defVar;
        if(CallInst *callInst = dyn_cast<CallInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(callInst->getArgOperand(op)))) return NULL;               
        }

        else if(auto *retInst = dyn_cast<ReturnInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(retInst->getReturnValue()))) return NULL;
        }

        else if(auto *storeInst = dyn_cast<StoreInst>(i)){
            if(!(defVar = dyn_cast<Instruction>(storeInst->getValueOperand()))) return NULL;
        }

        if(CallInst *defCall = dyn_cast<CallInst>(defVar)){            
            if(summary.find(defCall->getCalledFunction()) != summary.end()){
                if(summary[defCall->getCalledFunction()].funcReturnVal != NULL){
                    ConstantInt* const1 = dyn_cast<ConstantInt>(summary[defCall->getCalledFunction()].funcReturnVal);
                    errs()<<"\nFound a constant: "<<const1->getSExtValue()<<" to propogate from callee: "<<defCall->getCalledFunction()->getName();
                    errs()<<" for instruction: ";
                    i->print(errs());
                    constants.push_back(const1);
                }
            }
        }

        if (mayMustAliases.find(defVar) != mayMustAliases.end()){
            for (auto aliasInst : mayMustAliases[defVar]){
                if (!decideToPropogate(i, op, aliasInst, constants )){
                    return NULL;
                }
            }
        }
        else{
            if (!decideToPropogate(i, op, NULL, constants )){
                return NULL;
            }
        }

        Value* constant_to_propogate = NULL;
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

    bool decideToPropogate(Instruction* i, int op, Instruction* aliasInst, std::vector<ConstantInt *> &constants){


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

        for(auto in : inSet[i].set_bits()){
            if(CallInst *callInst_in = dyn_cast<CallInst>(Insts[in])){
                if(callInst_in->getCalledFunction()->getName() == "CAT_new"){
                    if( ((Insts[in] == defVar) && (!isEscapedVar(defVar))) || ((Insts[in] == aliasInst) && (!isEscapedVar(aliasInst))) ){   

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
                        if(  ((setVar == defVar) && (!isEscapedVar(defVar))) || ((setVar == aliasInst) && (!isEscapedVar(aliasInst)))){                             
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
                if((Insts[in] == defVar) || (Insts[in] == aliasInst)){
                    if(!isEscapedVar(Insts[in])){
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
        }
        return true;
    }

    bool isEscapedVar(Instruction* defVar){ //Check if the variable escapes(ModRef) the function  since this is a conservative intra-procedural pass
        // errs()<<"\n isEscapedVar check for: \t";
        // defVar->print(errs());
        
        bool varEscapes = false;
        for(auto &U  : defVar->uses()){
      //       errs()<<"\n use: \t";
      //       U.getUser()->print(errs());
            // errs()<<"\n";
            if(auto *useVar = dyn_cast<CallInst>(U.getUser())){
                if((useVar->getCalledFunction()->getName() != "CAT_new") &&
                    (useVar->getCalledFunction()->getName() != "CAT_add") &&
                    (useVar->getCalledFunction()->getName() != "CAT_sub") &&
                    (useVar->getCalledFunction()->getName() != "CAT_set") &&
                    (useVar->getCalledFunction()->getName() != "CAT_get")){
                    switch(aliasAnalysis->getModRefInfo(useVar ,(CallInst*)defVar)){
                        case ModRefInfo::Mod: 
                        case ModRefInfo::Ref: 
                        case ModRefInfo::ModRef: 
                        case ModRefInfo::MustMod: 
                        case ModRefInfo::MustRef: 
                        case ModRefInfo::MustModRef:
                            return true;
                        default:
                            break;
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
            errs()<<"\n Replaced all uses of instruction: "; 
            i.first->print(errs());  
            errs()<<" with value "<<const1->getSExtValue()<<" by Constant Propogation";               
            BasicBlock::iterator ii(i.first);
            ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);                    
        }
    }
    bool replaceFoldedConstants(){ //Check and replace all uses of folded Constants

        bool folded = false;
        for(auto &i : foldedConstants){
            std::vector<Use *> toChange;
            CallInst* foldedInst = dyn_cast<CallInst>(i.first);
            Value* foldedVar = foldedInst->getArgOperand(0);
            if(auto* foldedVarDef = dyn_cast<CallInst>(foldedVar)){
                for (auto &U : foldedVarDef->uses()) {
                    std::vector<CallInst *> reachingDefs;
                    User* user = U.getUser();
                    if (auto *useInst = dyn_cast<CallInst>(user)){
                        if(useInst->getCalledFunction()->getName() == "CAT_get"){           
                            for(auto in : inSet[useInst].set_bits()){

                                if(auto* callInst = dyn_cast<CallInst>(Insts[in])){
                                    if((callInst->getCalledFunction()->getName() == "CAT_add") || 
                                        (callInst->getCalledFunction()->getName() == "CAT_sub") || 
                                        (callInst->getCalledFunction()->getName() == "CAT_set")) {
                                        if(foldedVar == callInst->getArgOperand(0)){
                                            reachingDefs.push_back(callInst);
                                        }
                                    }
                                    else if(callInst->getCalledFunction()->getName() == "CAT_new"){
                                        if(foldedVar == callInst){
                                            reachingDefs.push_back(callInst);
                                        }
                                    }
                                }
                            }
                            if(reachingDefs.size() == 1){
                                if(reachingDefs[0] == foldedInst){
                                    toChange.push_back(&U);
                                }
                            }
                        }
                    }
                }
            }
            if(!toChange.empty()){
                for (auto *U : toChange){
                    ConstantInt* const1 = dyn_cast<ConstantInt>(i.second);
                    errs()<<"\n Replaced all uses of instruction:";
                    U->getUser()->print(errs());       
                    errs()<<" with value "<<const1->getSExtValue()<<" by Constant Folding";                    
                               
                    CallInst* toReplace = dyn_cast<CallInst>(U->getUser());
                    BasicBlock::iterator ii(toReplace);
                    ReplaceInstWithValue(toReplace->getParent()->getInstList(), ii, i.second);
                }
                folded = true;
            }   
        }
        return folded;
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

                            if (isDeadCall(callInst)){
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

    bool isDeadCall(CallInst* callInst){
        bool escapedCall = false;
        Function* calleeF = callInst->getCalledFunction();
        
        if(calleeF->isDeclaration()){
            if((calleeF->getName() == "CAT_new") ||
                (callInst->getCalledFunction()->getName() == "CAT_get")){
                return true;
            }
            else{
                return false;
            }                
        }

        else if(callInst->getNumUses() == 0){
            for(auto &bb : *calleeF){
                for(auto &i : bb){
                    if(CallInst *newCall = dyn_cast<CallInst>(&i)){
                       if(!isDeadCall(newCall)){
                            escapedCall = true;
                        }
                    }
                }
            }

            return !escapedCall;

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
                llvmFoldedConstants.erase(i.first);
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
            errs()<<"\n***************** GEN\n{\n";
            printOutputSet(genSet[Insts[i]], Insts);
            errs() << "}\n**************************************\n***************** KILL\n{\n";
            printOutputSet(killSet[Insts[i]], Insts);
            errs() << "}\n**************************************\n\n\n\n";
            errs() << "}\n**************************************\n***************** IN\n{\n";
            printOutputSet(inSet[Insts[i]], Insts);
            errs() << "}\n**************************************\n***************** OUT\n{\n";
            printOutputSet(outSet[Insts[i]], Insts);
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


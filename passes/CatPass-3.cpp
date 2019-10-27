#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <vector>
#include <unordered_map>
#include <set>

using namespace llvm;

namespace {
  struct CAT : public FunctionPass {
    static char ID; 
    CAT() : FunctionPass(ID) {}
    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point
    bool doInitialization (Module &M) override {

      //errs() << "Hello LLVM World at \"doInitialization\"\n" ;
      return false;
    }

    // This function is invoked once per function compiled
    // The LLVM IR of the input functions is ready and it can be analyzed and/or transformed
    bool runOnFunction (Function &F) override {
        std::vector<Instruction *> Insts;
        std::unordered_map<Instruction *, llvm::BitVector> genSet;
        std::unordered_map<Instruction *, llvm::BitVector> killSet;
        std::unordered_map<Instruction *, llvm::BitVector> inSet;
        std::unordered_map<Instruction *, llvm::BitVector> outSet;
        
        for(auto &bb : F){
            for(auto &i : bb){
                Insts.push_back(&i);
            }
        } 

        for (int i = 0; i < Insts.size(); i++){
            genSet[Insts[i]] = BitVector(Insts.size(), false);
            killSet[Insts[i]] = BitVector(Insts.size(), false); 
            inSet[Insts[i]] = BitVector(Insts.size(), false);
            outSet[Insts[i]] = BitVector(Insts.size(), false);
        }

        computeGenKill(F, genSet, killSet, Insts);
        
        computeInOut(F, genSet, killSet, inSet, outSet, Insts);

        errs() << "START FUNCTION: " << F.getName() << '\n';
        for (int i = 0; i < Insts.size(); i++){
            errs()<<"INSTRUCTION: ";  
            Insts[i]->print(errs());
            errs()<<"\n***************** IN\n{\n";
            printOutputSet(inSet[Insts[i]], Insts);
            errs() << "}\n**************************************\n***************** OUT\n{\n";
            printOutputSet(outSet[Insts[i]], Insts);
            errs() << "}\n**************************************\n\n\n\n";
        }

        Insts.clear();
        genSet.clear();
        killSet.clear();
        inSet.clear();
        outSet.clear();
        return false;
    }

    void computeGenKill(Function &F, std::unordered_map<Instruction *, llvm::BitVector> &genSet, std::unordered_map<Instruction *, llvm::BitVector> &killSet, std::vector<Instruction *> &Insts){
        int instCount = 0;
        for(auto &bb : F){
            for(auto &i : bb){
                if(CallInst *callInst = dyn_cast<CallInst>(&i)){
                    Function *callee = callInst->getCalledFunction();
                    if((callee->getName() == "CAT_add") || (callee->getName() == "CAT_sub") || (callee->getName() == "CAT_set")){
                        genSet[&i].set(instCount);
                        if(auto *Operand = dyn_cast<Instruction>(callInst->getArgOperand(0))){
                           for(int idx =0; idx<Insts.size(); idx++ ){
                                if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                    Function *curCallee = curInst->getCalledFunction();
                                    if(curCallee->getName() == "CAT_new"){
                                        if(Operand == Insts[idx]){
                                            killSet[&i].set(idx);
                                        }         
                                    }
                                    if((curCallee->getName() == "CAT_add") || 
                                        (curCallee->getName() == "CAT_sub") || 
                                        (curCallee->getName() == "CAT_set")){
                                        if(auto *curOperand = dyn_cast<Instruction>(curInst->getArgOperand(0))){
                                            if(curOperand == Operand){
                                                killSet[&i].set(idx);
                                            }
                                        }
                                    }
                               }
                            }
                        }
                        killSet[&i].reset(instCount);
                    }
                    if(callee->getName() == "CAT_new"){
                        genSet[&i].set(instCount);
                        for(int idx =0; idx<Insts.size(); idx++ ){
                            if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                Function *curCallee = curInst->getCalledFunction();
                                if((curCallee->getName() == "CAT_add") || 
                                    (curCallee->getName() == "CAT_sub") || 
                                    (curCallee->getName() == "CAT_set")){
                                    if(auto *curOperand = dyn_cast<Instruction>(curInst->getArgOperand(0))){
                                        if(curOperand == &i){
                                            killSet[&i].set(idx);
                                        }
                                    }
                                }
                            }
                        }
                        killSet[&i].reset(instCount);
                    }
                }
                instCount++;
            }
        }
    }

    void computeInOut(Function &F, std::unordered_map<Instruction *, llvm::BitVector> &genSet, std::unordered_map<Instruction *, llvm::BitVector> &killSet, std::unordered_map<Instruction *, llvm::BitVector> &inSet, std::unordered_map<Instruction *, llvm::BitVector> &outSet,std::vector<Instruction *> &Insts){

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

    void printOutputSet(BitVector &set, std::vector<Instruction *> Insts){
        if(set.find_first() != -1){
            int idx = set.find_first();
            errs()<<" ";
            Insts[idx]->print(errs());
            errs()<<"\n";
            while(set.find_next(idx) != -1){
                idx = set.find_next(idx);
                errs()<<" ";
                Insts[idx]->print(errs());
                errs()<<"\n";
            }
        }
    }

    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      //errs() << "Hello LLVM World at \"getAnalysisUsage\"\n" ;
      AU.setPreservesAll();
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

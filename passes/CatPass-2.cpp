#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <vector>

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
        for(auto &bb : F){
            for(auto &i : bb){
                Insts.push_back(&i);
            }
        }
        std::vector<llvm::BitVector> genInsts;
        std::vector<llvm::BitVector> killInsts;
        int instCount = 0;
    	for(auto &bb : F){
    		for(auto &i : bb){
                genInsts.push_back(BitVector(Insts.size(), false));
                killInsts.push_back(BitVector(Insts.size(), false)); 
    			if(CallInst *callInst = dyn_cast<CallInst>(&i)){
    				Function *callee = callInst->getCalledFunction();
    				if((callee->getName() == "CAT_add") || (callee->getName() == "CAT_sub") || (callee->getName() == "CAT_set")){
                        genInsts.back().set(instCount);
                        if(auto *Operand = dyn_cast<Instruction>(callInst->getArgOperand(0))){
                           for(int idx =0; idx<Insts.size(); idx++ ){
                                if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                    Function *curCallee = curInst->getCalledFunction();
                                    if(curCallee->getName() == "CAT_new"){
                                        if(Operand == Insts[idx]){
                                            killInsts.back().set(idx);
                                        }         
                                    }
                                    if((curCallee->getName() == "CAT_add") || 
                                        (curCallee->getName() == "CAT_sub") || 
                                        (curCallee->getName() == "CAT_set")){
                                        if(auto *curOperand = dyn_cast<Instruction>(curInst->getArgOperand(0))){
                                            if(curOperand == Operand){
                                                killInsts.back().set(idx);
                                            }
                                        }
                                    }
                               }
                            }
                        }
                        killInsts.back().reset(instCount);
                    }
    				if(callee->getName() == "CAT_new"){
                        genInsts.back().set(instCount);
                        for(int idx =0; idx<Insts.size(); idx++ ){
                            if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                Function *curCallee = curInst->getCalledFunction();
                                 if(curCallee->getName() == "CAT_new"){
                                    if(Insts[idx] == &i){
                                        killInsts.back().set(idx);
                                    }         
                                }
                                if((curCallee->getName() == "CAT_add") || 
                                    (curCallee->getName() == "CAT_sub") || 
                                    (curCallee->getName() == "CAT_set")){
                                    if(auto *curOperand = dyn_cast<Instruction>(curInst->getArgOperand(0))){
                                        if(curOperand == &i){
                                            killInsts.back().set(idx);
                                        }
                                    }
                                }
                            }
                        }
                        killInsts.back().reset(instCount);
                    }
    			}
                instCount++;
    		}
    	}

        errs() << "START FUNCTION: " << F.getName() << '\n';
        for (int i = 0; i < Insts.size(); i++){
            errs()<<"INSTRUCTION: ";  
            Insts[i]->print(errs());
            errs()<<"\n***************** GEN\n{\n";
            if(genInsts[i].find_first() != -1){
                int gen = genInsts[i].find_first();
                errs()<<" ";
                Insts[gen]->print(errs());
                errs()<<"\n";
                while(genInsts[i].find_next(gen) != -1){
                    gen = genInsts[i].find_next(gen);
                    errs()<<" ";
                    Insts[gen]->print(errs());
                    errs()<<"\n";
                }
            }
            errs() << "}\n**************************************\n***************** KILL\n{\n";
            if(killInsts[i].find_first() != -1){
                int kill = killInsts[i].find_first();
                errs()<<" ";
                Insts[kill]->print(errs());
                errs()<<"\n";
                while(killInsts[i].find_next(kill) != -1){
                    kill = killInsts[i].find_next(kill);
                    errs()<<" ";
                    Insts[kill]->print(errs());
                    errs()<<"\n";
                }
            }
            errs() << "}\n**************************************\n\n\n\n";
        }
        Insts.clear();
        genInsts.clear();
        killInsts.clear();
        return false;
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

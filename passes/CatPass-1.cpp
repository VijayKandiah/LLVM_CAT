#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
  struct CAT : public FunctionPass {
    static char ID; 

    CAT() : FunctionPass(ID) {}
    int cat_add, cat_sub, cat_new, cat_get, cat_set = 0;
    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point
    bool doInitialization (Module &M) override {

      //errs() << "Hello LLVM World at \"doInitialization\"\n" ;
      return false;
    }

    // This function is invoked once per function compiled
    // The LLVM IR of the input functions is ready and it can be analyzed and/or transformed
    bool runOnFunction (Function &F) override {
    	cat_add = cat_sub = cat_new = cat_get = cat_set = 0;
    	for(auto &bb : F){
    		for(auto &i : bb){
    			if(CallInst *callInst = dyn_cast<CallInst>(&i)){
    				Function *callee = callInst->getCalledFunction();
    				if(callee->getName() == "CAT_add") cat_add++;
    				else if(callee->getName() == "CAT_sub") cat_sub++;
    				else if(callee->getName() == "CAT_new") cat_new++;
    				else if(callee->getName() == "CAT_get") cat_get++;
    				else if(callee->getName() == "CAT_set") cat_set++;	
    			}
    		}
    	}
    	if (cat_add) errs() << "H1: \""<<F.getName() << "\": CAT_add: " << cat_add <<"\n" ;
    	if (cat_sub) errs() << "H1: \""<<F.getName() << "\": CAT_sub: " << cat_sub <<"\n" ;
    	if (cat_new) errs() << "H1: \""<<F.getName() << "\": CAT_new: " << cat_new <<"\n" ;
    	if (cat_get) errs() << "H1: \""<<F.getName() << "\": CAT_get: " << cat_get <<"\n" ;
    	if (cat_set) errs() << "H1: \""<<F.getName() << "\": CAT_set: " << cat_set <<"\n" ;
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

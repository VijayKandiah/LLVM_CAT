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
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Use.h"
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
        std::unordered_map<Instruction*, Value*> propogatedConstants;
        std::unordered_map<Instruction*, Value*> foldedConstants;
        bool modified = false;
        bool propogated = true;
        bool folded = true;


        while(propogated | folded){
        	Insts.clear();
		    genSet.clear();
		    killSet.clear();
		    inSet.clear();
		    outSet.clear();
		    propogatedConstants.clear();
		    foldedConstants.clear();
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

		    
		    propogated = false;
		    propogated = constantPropogation(inSet, Insts, propogatedConstants);

		    if(propogated){
			    for(auto &i : propogatedConstants){
					BasicBlock::iterator ii(i.first);
					ReplaceInstWithValue(i.first->getParent()->getInstList(), ii, i.second);
					modified = true;
				}
				continue;
			}

		    folded = false;
		    folded = constantFolding(inSet, Insts, foldedConstants);
		    if(folded){
		    	folded = replaceFoldedUses(inSet, Insts, foldedConstants);
		    	if(folded){
		    		modified = true;
                }
				continue;
		    }
		}

		bool deletedInsts = deadCodeElimination(F);

        return modified | deletedInsts;
    }


    void computeGenKill(Function &F, 
    	std::unordered_map<Instruction *, llvm::BitVector> &genSet, 
    	std::unordered_map<Instruction *, llvm::BitVector> &killSet, 
    	std::vector<Instruction *> &Insts){

        int instCount = 0;
        for(auto &bb : F){
            for(auto &i : bb){
                if(CallInst *callInst = dyn_cast<CallInst>(&i)){
                    if((callInst->getCalledFunction()->getName() == "CAT_add") || 
                    	(callInst->getCalledFunction()->getName() == "CAT_sub") || 
                    	(callInst->getCalledFunction()->getName() == "CAT_set")){
                        genSet[&i].set(instCount);
                        
                       	for(int idx =0; idx<Insts.size(); idx++ ){
                            if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                if(curInst->getCalledFunction()->getName() == "CAT_new"){
                                	if(auto *Operand = dyn_cast<Instruction>(callInst->getArgOperand(0))){
	                                    if(Operand == Insts[idx]){
	                                        killSet[&i].set(idx);
	                                    }  
	                                }       
                                }
                                if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                                    (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                                    (curInst->getCalledFunction()->getName() == "CAT_set")){                                    
                                    if(curInst->getArgOperand(0) == callInst->getArgOperand(0)){
                                        killSet[&i].set(idx);
                                    }                                    
                                }
                           	}
                           	else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){
                           		for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                           			if(curInst->getIncomingValue(inc) == callInst->getArgOperand(0)){
                           				killSet[&i].set(idx);
                           			}
                           		}	
                           	}
                        }                         
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
                }
                killSet[&i].reset(instCount);
                instCount++;
            }
        }
    }


    void computeInOut(Function &F, 
    	std::unordered_map<Instruction *, llvm::BitVector> &genSet, 
    	std::unordered_map<Instruction *, llvm::BitVector> &killSet, 
    	std::unordered_map<Instruction *, llvm::BitVector> &inSet, 
    	std::unordered_map<Instruction *, llvm::BitVector> &outSet,
    	std::vector<Instruction *> &Insts){

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


    bool constantPropogation(std::unordered_map<Instruction *, llvm::BitVector> &inSet,
    	std::vector<Instruction *> &Insts, 
    	std::unordered_map<Instruction*, Value*> &propogatedConstants){

    	bool modified = false;
    	for (auto i : Insts){
    		if(CallInst *callInst = dyn_cast<CallInst>(i)){
    			Value *constant_to_propogate = NULL;
                if(callInst->getCalledFunction()->getName() == "CAT_get"){
                   	constant_to_propogate = decideToPropogate(inSet, Insts, i,  0);
                   	if(constant_to_propogate != NULL){
                   		propogatedConstants[i] = constant_to_propogate;
                   		modified = true;
                	}
                }
            }
    	}
    	return modified;
    }


    bool constantFolding(std::unordered_map<Instruction *, llvm::BitVector> &inSet,
    	std::vector<Instruction *> &Insts,
    	std::unordered_map<Instruction*, Value*> &foldedConstants){

    	bool modified = false;
    	for (auto i : Insts){
    		if(CallInst *callInst = dyn_cast<CallInst>(i)){
    			Value *constant_to_propogate = NULL;
	            if((callInst->getCalledFunction()->getName() == "CAT_add") || (callInst->getCalledFunction()->getName() == "CAT_sub")){            	
                	Value *op1 = decideToPropogate(inSet, Insts, i, 1);
                	Value *op2 = decideToPropogate(inSet, Insts, i, 2);
      
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

    
     bool replaceFoldedUses(std::unordered_map<Instruction *, llvm::BitVector> &inSet,
    	std::vector<Instruction *> &Insts,
    	std::unordered_map<Instruction*, Value*> &foldedConstants){

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
                    errs()<<"\n TO REPLACE: ";
                    U->getUser()->print(errs());
                    ConstantInt* const1 = dyn_cast<ConstantInt>(i.second);
                    errs()<<"with "<<const1->getSExtValue();
            		CallInst* toReplace = dyn_cast<CallInst>(U->getUser());
            		BasicBlock::iterator ii(toReplace);
					ReplaceInstWithValue(toReplace->getParent()->getInstList(), ii, i.second);
				}
				folded = true;
    		}	
		}
		return folded;
	}

	Value* decideToPropogate(std::unordered_map<Instruction *, llvm::BitVector> &inSet,
    	std::vector<Instruction *> &Insts,
    	Instruction* i, 
    	int op){

    	std::vector<Instruction *> reachingDefs;
    	Value *constant_to_propogate = NULL;
    	CallInst *callInst = dyn_cast<CallInst>(i);
        Function *callee = callInst->getCalledFunction();
        
    	for(auto in : inSet[i].set_bits()){
    		if(CallInst *callInst_in = dyn_cast<CallInst>(Insts[in])){
    			if(callInst_in->getCalledFunction()->getName() == "CAT_new"){
    				if(auto *defVar = dyn_cast<Instruction>(callInst->getArgOperand(op))){
	    				if((Insts[in] == defVar) && (!isEscapedVar(defVar))){	    					    						    					
    						reachingDefs.push_back(callInst_in);
	    					if(isa<ConstantInt>(callInst_in->getArgOperand(0))){
	    						constant_to_propogate = callInst_in->getArgOperand(0);
	    					}
	    					else{
	    						constant_to_propogate = NULL;
	    					}		    				
	    				}
	    			}
    			}
    			else if(callInst_in->getCalledFunction()->getName() == "CAT_set"){
    				if(auto *setVar = dyn_cast<Instruction>(callInst_in->getArgOperand(0))){
    					if(auto *defVar = dyn_cast<Instruction>(callInst->getArgOperand(op))){
	    					if((setVar == defVar) && (!isEscapedVar(defVar))){	    						
	    						reachingDefs.push_back(callInst_in);
	    						if(isa<ConstantInt>(callInst_in->getArgOperand(1))){
	    							constant_to_propogate = callInst_in->getArgOperand(1);
	    						}
	    						else{
	    							constant_to_propogate = NULL;
	    						}		    					
	    					}
	    				}
    				}
    			}
    			else if((callInst_in->getCalledFunction()->getName() == "CAT_add") || (callInst_in->getCalledFunction()->getName() == "CAT_sub")){
    				if(auto *setVar = dyn_cast<Instruction>(callInst_in->getArgOperand(0))){
    					if(auto *defVar = dyn_cast<Instruction>(callInst->getArgOperand(op))){
	    					if(setVar == defVar){
	    						reachingDefs.push_back(callInst_in);
	    						constant_to_propogate = NULL;
	    						break;
	    					}
	    				}
    				}
    			}
    		}
    		else if(PHINode *curInst = dyn_cast<PHINode>(Insts[in])){
    			ConstantInt* PHIvalue = NULL;
    			if(auto *defVar = dyn_cast<Instruction>(callInst->getArgOperand(op))){
    				if(Insts[in] == defVar){
						for(int index = 0; index < curInst->getNumIncomingValues(); index++){					
							if(auto *defVar = dyn_cast<CallInst>(curInst->getIncomingValue(index))){
								if(defVar->getCalledFunction()->getName()== "CAT_new"){
									if(auto *constantVar = dyn_cast<ConstantInt>(defVar->getArgOperand(0))){
			    						if(PHIvalue == NULL){
			    							PHIvalue = dyn_cast<ConstantInt>(defVar->getArgOperand(0));
			    						}
			    						else if(PHIvalue->getSExtValue() == constantVar->getSExtValue()){
			    							constant_to_propogate = defVar->getArgOperand(0);
			    						}
			    						else{
			    							constant_to_propogate = NULL;
			    							PHIvalue = NULL;
			    						}
			    					}
								}
							}					
						}
						reachingDefs.push_back(curInst);
					}
				}				                      	
    		}
    	}
       	

       	if(reachingDefs.size() == 1)
       		return constant_to_propogate;
       	else
       		return NULL;
    }

    bool isEscapedVar(Instruction* defVar){

		for(auto &U  : defVar->uses()){
			if(auto *useVar = dyn_cast<CallInst>(U.getUser())){
				if((useVar->getCalledFunction()->getName() != "CAT_add") &&
					(useVar->getCalledFunction()->getName() != "CAT_sub") &&
					(useVar->getCalledFunction()->getName() != "CAT_set") &&
					(useVar->getCalledFunction()->getName() != "CAT_get")){
					return true;
					
				}	    							
			}
			else if(auto *storeInst = dyn_cast<StoreInst>(U.getUser())){
				if(auto *callStore = dyn_cast<CallInst>(storeInst->getValueOperand())){
					if(callStore->getCalledFunction()->getName() == "CAT_new"){
						return true;
						
					}
				}
			}
		}
		return false;
    }

    bool deadCodeElimination(Function &F){

    	bool flag = true;
    	bool modified = true;
		std::set<Instruction *> toDelete;
		while(flag){
			flag = false;
			for(auto &bb : F){
		        for(auto &i : bb){
	    			if(CallInst *callInst = dyn_cast<CallInst>(&i)){
	                    if(callInst->getCalledFunction()->getName() == "CAT_new"){
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
		                    					//continue;
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
                errs()<<"\n TO DELETE: ";
                I->print(errs());
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

    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override {
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

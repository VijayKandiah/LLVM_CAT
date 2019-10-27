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

    std::vector<Instruction *> Insts;
    std::unordered_map<Instruction *, llvm::BitVector> genSet, killSet, inSet, outSet; 
    std::unordered_map<Instruction*, Value*> propogatedConstants;
    std::unordered_map<Instruction*, Value*> foldedConstants;
    AliasAnalysis *aliasAnalysis;
    std::unordered_map<Instruction *,  std::set<Instruction*>> mayMustAliases, mustAliases;
    std::set< StoreInst* > escapedStores;  
    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point
    bool doInitialization (Module &M) override {

      return false;
    }

    // This function is invoked once per function compiled
    // The LLVM IR of the input functions is ready and it can be analyzed and/or transformed
    bool runOnFunction (Function &F) override {

        bool modified = false;
        bool propogated = true;
        bool folded = true;

        aliasAnalysis = &(getAnalysis< AAResultsWrapperPass >().getAAResults());
        while(propogated | folded){

        	initializeSets(F);

            computeAliases(F);

		    computeGenKill(F);
		    
		    computeInOut(F);
		    
		    propogated = false;
		    propogated = constantPropogation();

		    if(propogated){
		    	replacePropogatedConstants();
		    	modified = true;			    
				continue;
			}

		    folded = false;
		    folded = constantFolding();

		    if(folded){
		    	folded = replaceFoldedConstants();
		    	modified != folded;
				continue;
		    }
		}
		modified |= deadCodeElimination(F);
        return modified;
    }

    void initializeSets(Function &F){
        Insts.clear();
        genSet.clear();
        killSet.clear();
        inSet.clear();
        outSet.clear();
        propogatedConstants.clear();
        foldedConstants.clear();
        mayMustAliases.clear();
        mustAliases.clear();

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
    }

    void computeAliases(Function &F){

        std::vector< Instruction* > loadInsts;
        std::vector< StoreInst* > storeInsts;
        std::set< CallInst* > nonCATCalls;
        for (auto I : Insts){
            if(auto *call = dyn_cast<CallInst>(I)){
                if( (call->getCalledFunction()->getName() != "CAT_new") &&
                    (call->getCalledFunction()->getName() != "CAT_add") &&
                    (call->getCalledFunction()->getName() != "CAT_sub") &&
                    (call->getCalledFunction()->getName() != "CAT_set") &&
                    (call->getCalledFunction()->getName() != "CAT_get")){

                    nonCATCalls.insert(call);                    
                }                                   
            }
            if ( isa<LoadInst>(I)){
                loadInsts.push_back(I);
                continue;
            }
            if (auto store = dyn_cast<StoreInst>(I)){
                storeInsts.push_back(store);
                continue;
            }
        }
        

        for(auto store : storeInsts){
            for (auto load : loadInsts){
                switch (aliasAnalysis->alias(MemoryLocation::get(store), MemoryLocation::get(load))){
                    case MustAlias:
                        {
                            Instruction* storedVal = dyn_cast<Instruction>(store->getValueOperand());
                            mustAliases[storedVal].insert(load);
                            mustAliases[load].insert(storedVal);
                            mayMustAliases[storedVal].insert(load);
                            mayMustAliases[load].insert(storedVal);
                            break;
                        }
                    case MayAlias: case PartialAlias:
                        {
                            Instruction* storedVal = dyn_cast<Instruction>(store->getValueOperand());
                            mayMustAliases[storedVal].insert(load);
                            mayMustAliases[load].insert(storedVal);
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
                    if(call->getOperand(i) == store->getPointerOperand()){
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

                        if(auto *defInst = dyn_cast<Instruction>(callInst->getArgOperand(0))){
                           	for(int idx =0; idx<Insts.size(); idx++ ){
                                if(CallInst *curInst = dyn_cast<CallInst>(Insts[idx])){
                                    if(curInst->getCalledFunction()->getName() == "CAT_new"){                                	
                                        if(defInst == Insts[idx]){
                                            killSet[&i].set(idx);
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

                           markAliasedSets(&i, defInst); // Gen[i] = Gen[i] U MayAlias[i]->redefinitions U MustAlias[i]->redefinitions 
                            							  // Kill[i] = Kill[i] U MustAlias[i]->redefinitions                    
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

                        markAliasedSets(&i, &i); // Gen[i] = Gen[i] U MayAlias[i]->redefinitions U MustAlias[i]->redefinitions 
                            					 // Kill[i] = Kill[i] U MustAlias[i]->redefinitions 
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

                    markAliasedSets(&i, &i);   // Gen[i] = Gen[i] U MayAlias[i]->redefinitions U MustAlias[i]->redefinitions 
                            					// Kill[i] = Kill[i] U MustAlias[i]->redefinitions    
                }

                killSet[&i].reset(instCount);
                instCount++;
            }
        }
    }

    void markAliasedSets(Instruction* i, Instruction* defInst){

    	//	mark all 'must' aliases of I and their redefinitions (through CAT_adds, PHINodes, etc) in the KILL set of I
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
        //	mark all 'may' and 'must' aliases (load instructions) of I and their redefinitions (through CAT_add, PHINode for ex.) in the GEN set of I
        if (mayMustAliases.find(defInst) != mayMustAliases.end()){
            for (auto aliasInst : mayMustAliases[defInst]){
                for(int idx =0; idx<Insts.size(); idx++ ){
                    if(Insts[idx] == aliasInst){
                        genSet[i].set(idx); 
                    }
                    if(auto *curInst = dyn_cast<CallInst>(Insts[idx])){
                        if((curInst->getCalledFunction()->getName() == "CAT_add") || 
                            (curInst->getCalledFunction()->getName() == "CAT_sub") || 
                            (curInst->getCalledFunction()->getName() == "CAT_set")){                                                                        
                            if(curInst->getArgOperand(0) == aliasInst){
                               genSet[i].set(idx);
                            }
                        } 
                    }
                    else if(PHINode *curInst = dyn_cast<PHINode>(Insts[idx])){                                
                        for(int inc = 0; inc < curInst->getNumIncomingValues(); inc++){
                            if(curInst->getIncomingValue(inc) == aliasInst){
                                genSet[i].set(idx);
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
                   	constant_to_propogate = decideToPropogate(i,  0);
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
                	Value *op1 = decideToPropogate(i, 1);
                	Value *op2 = decideToPropogate(i, 2);
      
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

    void replacePropogatedConstants(){  //Replace all uses of propogated Constants
    	for(auto &i : propogatedConstants){
			ConstantInt* const1 = dyn_cast<ConstantInt>(i.second);
            errs()<<"\n Replaced all uses of this instruction with value "<<const1->getSExtValue()<<" by Constant Propogation:";
            i.first->print(errs());                  
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
            		errs()<<"\n Replaced all uses of this instruction with value "<<const1->getSExtValue()<<" by Constant Folding:";                    
                    U->getUser()->print(errs());                  
            		CallInst* toReplace = dyn_cast<CallInst>(U->getUser());
            		BasicBlock::iterator ii(toReplace);
					ReplaceInstWithValue(toReplace->getParent()->getInstList(), ii, i.second);
				}
				folded = true;
    		}	
		}
		return folded;
	}

	Value* decideToPropogate(Instruction* i, int op){ //Check for constant propogation possibility for a CAT_get instruction i

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
                            else{
                                if(mayMustAliases.find(defVar) != mayMustAliases.end()){
                                    if(mayMustAliases[defVar].find(setVar) != mayMustAliases[defVar].end()){
                                        reachingDefs.push_back(callInst_in);
                                        constant_to_propogate = NULL;
                                        break; 
                                    }
                                } 
                            }
	    				}
    				}
    			}
    		}
    		else if(PHINode *curInst = dyn_cast<PHINode>(Insts[in])){   			
    			ConstantInt* PHIvalue = NULL;
    			if(auto *defVar = dyn_cast<Instruction>(callInst->getArgOperand(op))){
    				if(Insts[in] == defVar){
    					if(!isEscapedVar(Insts[in])){
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
    	}
       	

       	if(reachingDefs.size() == 1)
       		return constant_to_propogate;
       	else
       		return NULL;
    }

    bool isEscapedVar(Instruction* defVar){ //Check if the variable escapes(ModRef) the function  since this is a conservative intra-procedural pass

		for(auto &U  : defVar->uses()){
			if(auto *useVar = dyn_cast<CallInst>(U.getUser())){
				if((useVar->getCalledFunction()->getName() != "CAT_add") &&
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
                    return true;					
				}
				else{
					checkEscapedStores(storeInst); 
				}
			}
		}
		return false;
    }

    bool checkEscapedStores(StoreInst* defStore){ //recursively check for escaped stores
    	for(auto i : Insts){
			if(auto *storeInst = dyn_cast<StoreInst>(i)){
				if(storeInst->getValueOperand() == defStore->getPointerOperand()){
					if(escapedStores.find(storeInst) != escapedStores.end()) {
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

    bool deadCodeElimination(Function &F){ //Path-insensitive DCE pass

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
                errs()<<"\n Deleted this instruction by DCE: ";
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
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT()); }}); // ** for -O0

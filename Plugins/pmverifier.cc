#include "pmverifier.h"
#include "execution.h"
#include "action.h"
#include "model.h"
#include "clockvector.h"
#include "threads-model.h"
#include "cacheline.h"
#include "libpmem.h"

PMVerifier::~PMVerifier() {
	rangeMap.resetanddelete();
	beginRangeLastAction.resetanddelete();
	endRangeLastAction.resetanddelete();
	ignoreTable.resetanddelete();
	for(uint i=0;i< modelPairList.size();i++) {
		delete modelPairList[i];
	}
}

void PMVerifier::freeExecution(ModelExecution *exec) {
	ModelVector<Range*> *rangeVector = rangeMap.get(exec);
	if(rangeVector) {
		for(uint i=0;i< rangeVector->size();i++) {
			delete (*rangeVector)[i];
		}
		rangeVector->clear();
	}
	ModelVector<ModelAction*> *alist = beginRangeLastAction.get(exec);
	if(alist != NULL) {
		alist->clear();
		alist = NULL;
	}
	alist = endRangeLastAction.get(exec);
	if( alist != NULL) {
		alist->clear();
	}
}

/**
 * This simple analysis updates the endRange for all threads when an execution crashes.
 */
void PMVerifier::crashAnalysis(ModelExecution * execution) {
	ModelVector<Range*> *ranges = getOrCreateRangeVector(execution);
	for (unsigned int i = 0;i < execution->get_num_threads();i++) {
		thread_id_t tid = int_to_id(i);
		Range *range = getOrCreateRange(ranges, tid);
		ModelAction * action = execution->get_last_action(tid);
		if(action) {
			range->setEndRange(action->get_seq_number());
			setActionIndex( endRangeLastAction.get(execution), tid, action);
		} else {
			range->setEndRange(execution->get_curr_seq_num());
			setActionIndex(endRangeLastAction.get(execution), tid, execution->getLastAction());
		}
	}

}

void PMVerifier::populateWriteConstraint(Range &range, ModelAction *wrt, ModelExecution * wrtExecution, uintptr_t curraddress) {
	range.setBeginRange(wrt->get_seq_number());
	mllnode<ModelAction *> * nextNode = wrt->getActionRef()->getNext();
	while(nextNode != NULL) {
		ModelAction *tmp = nextNode->getVal();
		if(tmp->get_tid() == wrt->get_tid() && tmp->get_seq_number() > wrt->get_seq_number()) {
			uintptr_t bot = (uintptr_t)tmp->get_location();
			uintptr_t top = bot + tmp->getOpSize();
			if ((curraddress>=bot) && (curraddress <top)) {
				break;
			}
		}
		nextNode = nextNode->getNext();
	}
	if(nextNode) {
		ModelAction *nextWrite = nextNode->getVal();
		ASSERT(nextWrite);
		range.setEndRange(nextWrite->get_seq_number() - 1);
	} else {
		ModelAction * action = wrtExecution->get_last_action(wrt->get_tid());
		if(action) {
			range.setEndRange(action->get_seq_number());
		} else {
			range.setEndRange(wrtExecution->get_curr_seq_num());
		}
	}
}

void PMVerifier::printWriteAndFirstReadByThread(ModelExecution *exec, modelclock_t wclock, ModelAction *readThreadAction) {
	mllnode<ModelAction*> * actionNode = exec->getActionTrace()->getAction(wclock);
	ASSERT(actionNode);
	ModelAction *wrt = actionNode->getVal();
	wrt->print();
	mllnode<ModelAction *> *tmp = actionNode;
	ModelAction *tmpAction = NULL;
	while(true) {
		tmpAction = tmp->getVal();
		if(tmpAction->get_tid() == readThreadAction->get_tid() && tmpAction->get_cv()->getClock(wrt->get_tid()) == wclock) {
			break;
		}
		if(tmpAction->get_seq_number() > readThreadAction->get_seq_number()) {
			break;
		}
		tmp = tmp->getNext();
	}
	if(tmpAction!= wrt && tmpAction->get_seq_number() <= readThreadAction->get_seq_number()) {
		tmpAction->print();
	}
	if(!pmem_is_pmem(wrt->get_location(), wrt->getOpSize()<<3) ) {
		const char * location = wrt->get_position() ? wrt->get_position() : tmpAction && tmpAction->get_position() ? tmpAction->get_position() : "[]";
		model_print("(Warning: Variable at %s is located in DRAM. Either, it needs to be moved to persistent memory"
								" or use API to ignore robustness violations for this memory location!)\n", location);
	}
}

/**
 * This analysis checks all writes that the execution may possibly read from are in range.
 */
void PMVerifier::mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set) {
	if(disabled) {
		return;
	}
	model_params *params = model->getParams();
	ModelExecution *currExecution = model->get_execution();
	for(uint j=0;j< rf_set->size();j++) {
		SnapVector<Pair<ModelExecution *, ModelAction *> > * writeVec = (*rf_set)[j];
		bool hasError = false;
		for(uint i=0;i<read->getOpSize();i++ ) {
			ModelExecution * execution = (*writeVec)[i].p1;
			ModelAction *wrt = (*writeVec)[i].p2;
			if(currExecution != execution ) {
				// Check for persistency bugs
				model_print(">>>>>>> Possibly Reading from:\t");
				wrt->print();
				ModelVector<Range*> *rangeVector = getOrCreateRangeVector(execution);
				uint index = id_to_int(wrt->get_tid());
				Range *range = getOrCreateRange(rangeVector, index);
				uintptr_t curraddress = ((uintptr_t)wrt->get_location()) + i;
				Range writeRange;
				populateWriteConstraint(writeRange, wrt, execution, curraddress);
				if(!range->hastIntersection(writeRange) ) {
					if(model->getParams()->pmdebug > 0 ) {
						model_print("******************************\nPMVerifier found Robustness Violation in read:\n");
						read->print();
						model_print("From following write by thread %u:\n", wrt->get_tid());
						wrt->print();
						model_print("Write range:\t");
						writeRange.print();
						model_print("\tvs. Thread %d Range:\t", wrt->get_tid());
						range->print();
						model_print("\n");
						ModelAction *beginAction = getActionIndex( beginRangeLastAction.get(execution),index);
						if(beginAction) {
							model_print("Thread last begin Action:\n");
							beginAction->print();
						}
						model_print("Thread last end Action:\n");
						ModelAction * endAction = getActionIndex( endRangeLastAction.get(execution),index);
						endAction->print();
						model_print(">> Possible fix: Insert flushes after write(s):\n");
						if(writeRange.getEndRange() < range->getBeginRange()) {
							if(wrt->get_tid() != beginAction->get_tid()) {
								printWriteAndFirstReadByThread(execution, beginAction->get_cv()->getClock(wrt->get_tid()), beginAction);
							} else {
								wrt->print();
							}

						} else if(range->getEndRange() < writeRange.getBeginRange()) {
							beginAction->print();
						}
						model_print("****************************\n");
					}
					ERROR(execution, wrt, read, "Robustness Violation on Write");
					if(params->verifierPluginMode > 1) {
						hasError = true;
						break;
					}
				}
				ASSERT(wrt->get_cv());
				ClockVector *cv = wrt->get_cv();
				for(int i=0;i< cv->getNumThreads();i++) {
					range = getOrCreateRange(rangeVector, i);
					thread_id_t tid = int_to_id(i);
					modelclock_t trd_seq_num = cv->getClock(tid);
					if( ignoreVariable(execution->getActionTrace()->getAction(trd_seq_num)->getVal()->get_location()) ) {
						continue;
					}
					if(range->getEndRange() < trd_seq_num) {
						if(model->getParams()->pmdebug > 0 ) {
							model_print("******************************\nPMVerifier found Robustness Violation in read:\n");
							read->print();
							model_print("From write:\n");
							wrt->print();
							model_print("Conflicts in Thread %u Range:\t", tid);
							range->print();
							model_print("\n");
							ModelAction * beginAction = getActionIndex( beginRangeLastAction.get(execution),i);
							if(beginAction) {
								model_print("Thread last begin Action:\n");
								beginAction->print();
							}
							model_print("Thread last end Action:\n");
							ModelAction *endAction = getActionIndex( endRangeLastAction.get(execution),i);
							endAction->print();
							model_print(">> Possible fix: Insert flushes after write(s):\n");
							if( tid != wrt->get_tid() ) {
								printWriteAndFirstReadByThread(execution, trd_seq_num, wrt);
							}
							model_print("****************************\n");
						}

						ERROR(execution, wrt, read,
									"ERROR Persistency Bug on read from thread_id= %d\t with clock= %u out of range[%u,%u]\t");
						if(params->verifierPluginMode > 1 && !hasError) {
							hasError = true;
						}
					}
				}
				if( params->verifierPluginMode > 2 && !hasError && checkBeginRangeInversion(execution, wrt) & checkEndRangeInversion(execution, wrt, curraddress)) {
					hasError = true;
				}
			}
			if(hasError && params->verifierPluginMode > 1) {
				break;
			}
		}
		if(hasError && params->verifierPluginMode == 2) {
			// Exit mode. need to exit the execution when there is an error
			model->get_execution()->set_assert();
		} else if (hasError && params->verifierPluginMode == 3){
			// Safe mode. Not explore this write.
			rf_set->removeAt(j);
			j=j-1;
		}
	}
	if(rf_set->size()==0){
		// No more option to explore
		model->get_execution()->set_assert();
	}
}

bool PMVerifier::recordProgress(ModelExecution *exec, ModelAction *action) {
	if(disabled) {
		return true;
	}
	ModelVector<Range*> *rangeVector = getOrCreateRangeVector(exec);
	for(uint i=0;i< exec->get_num_threads();i++) {
		Range * range = getOrCreateRange(rangeVector, i);
		ClockVector * cv = action->get_cv();
		ASSERT(cv);
		thread_id_t tid = int_to_id(i);
		if(range->getEndRange()<cv->getClock(tid)) {
			if(model->getParams()->pmdebug > 0) {
				model_print("~~~~~~~~~ FATAL RANGE INVERSION ERROR ~~~~~~~~~~~~\n");
				range->print();
				model_print(">> Range Begin Action:\n");
				ModelAction *tmpact = getActionIndex(beginRangeLastAction.get(exec), i);
				if(tmpact) {
					tmpact->print();
				} else {
					model_print("No ModelAction Yet\n");
				}
				model_print(">> End Range Action:\n");
				tmpact = getActionIndex(endRangeLastAction.get(exec), i);
				tmpact->print();
				model_print(">> The write causing the bug:\n");
				action->print();
				model_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}
			if(!exec->has_asserted() && model->getParams()->verifierPluginMode > 1){
				exec->set_assert();
			}
			FATAL(exec, action, action, "RECORD PROGRESS: End range %u is not compatable with new begin range %u\n", range->getEndRange(), cv->getClock(tid));
		}
		if(range->canUpdateBeginRange(cv->getClock(tid))) {
			//This read is affecting begin Range of other threads. Need to check
			ModelAction *otherAction = exec->getActionTrace()->getAction(cv->getClock(tid))->getVal();
			if(ignoreVariable(otherAction->get_location())) {
				otherAction->print();
				if(tid == action->get_tid()) {
					return true;
				}
				continue;
			}
		}
		if(range->mergeBeginRange(cv->getClock(tid)) || (range->getBeginRange() == 0 && getActionIndex(beginRangeLastAction.get(exec), i) == NULL)) {
			setActionIndex(beginRangeLastAction.get(exec), i, action);
		}
	}
	return false;
}

/**
 * This analysis checks for the conflict and if it found none, updates the beginRange for all threads.
 * If the write is RMW, it finds the first next write in each thread and update their endRanges. Otherwise,
 * if the write was a normal write, it updates the threads' endRanges so they don't read from out of range
 * writes.
 */
void PMVerifier::readFromWriteAnalysis(ModelAction *read, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) {
	if(disabled) {
		return;
	}
	ModelExecution *currExecution = model->get_execution();
	void * address = read->get_location();
	for(uint i=0;i<read->getOpSize();i++ ) {
		uintptr_t curraddress = ((uintptr_t)address) + i;
		ModelExecution * execution = (*rfarray)[i].p1;
		if(currExecution != execution) {
			ModelAction *wrt = (*rfarray)[i].p2;
			if(model->getParams()->pmdebug > 1 ) {
				model_print("~~~~~~~~~~~~~~~~~READ FROM PREVIOUS WRITE~~~~~~~~~~~~~~~~~~~\n");
				read->print();
				model_print("Reading from write:\n");
				wrt->print();
				model_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}
			if(recordProgress(execution, wrt) ) {
				continue;
			}
			updateThreadsEndRangeafterWrite(execution, wrt, curraddress);
			crashInnerExecutionsBeforeFirstWrite(execution, curraddress);
		}

	}
}

void PMVerifier::crashInnerExecutionsBeforeFirstWrite(ModelExecution *execution, uintptr_t curraddress) {
	for(Execution_Context * pExecution = model->getPrevContext();pExecution != NULL ;pExecution=pExecution->prevContext) {
		ModelExecution * pexec = pExecution->execution;
		if(pexec == execution) {
			break;
		}
		updateThreadsEndRangeafterWrite(pexec, NULL, curraddress);
	}
}

void PMVerifier::findFirstWriteInEachThread(ModelVector<ModelAction*> &nextWrites, mllnode<ModelAction *> * node, uintptr_t curraddress, unsigned int numThreads) {
	uint count = 0;
	while(node!=NULL) {
		ModelAction *act = node->getVal();
		uintptr_t actbot = (uintptr_t) act->get_location();
		uintptr_t acttop = actbot + act->getOpSize();
		if ((curraddress>=actbot) && (curraddress <acttop)) {
			ASSERT(act->is_write());
			uint tid = id_to_int(act->get_tid());
			if(tid >= nextWrites.size()) {
				nextWrites.resize(tid +1);
			}
			if(nextWrites[tid] == NULL) {
				nextWrites[tid] = act;
				count++;
			}
			if(count == numThreads) {
				break;
			}
		}
		node=node->getNext();
	}
}

/**
 * Update the endRange to be before the first write after 'wrt' by each thread.
 * If wrt is null, this function returns the first write for each thread.
 */
void PMVerifier::updateThreadsEndRangeafterWrite(ModelExecution *execution, ModelAction *wrt, uintptr_t curraddress) {
	// Update the endRange for all threads
	mllnode<ModelAction *> * nextNode = NULL;
	if(wrt) {
		nextNode = wrt->getActionRef()->getNext();
	} else {
		void *addr = alignAddress( (void*)curraddress );
		simple_action_list_t * writes = execution->get_obj_write_map()->get(addr);
		if (writes == NULL)
			return;
		nextNode = writes->begin();
	}
	ModelVector<ModelAction*> nextWrites;
	findFirstWriteInEachThread(nextWrites, nextNode, curraddress, execution->get_num_threads());
	ModelVector<Range*> *ranges = getOrCreateRangeVector(execution);
	for(uint i=0;i < nextWrites.size();i++) {
		ModelAction *nextWrite = nextWrites[i];
		if(nextWrite) {
			Range *range = getOrCreateRange(ranges, i);
			if(range->getBeginRange()>nextWrite->get_seq_number()-1) {
				if(model->getParams()->pmdebug > 0) {
					model_print("~~~~~~~~~ FATAL RANGE INVERSION ERROR IN Setting EndRange ~~~~~~~~~~~~\n");
					range->print();
					model_print(">> Range Begin Action:\n");
					ModelAction *tmpact = getActionIndex(beginRangeLastAction.get(execution), i);
					if(tmpact) {
						tmpact->print();
					} else {
						model_print("No ModelAction Yet!\n");
					}
					model_print(">> End Range Action:\n");
					tmpact = getActionIndex(endRangeLastAction.get(execution), i);
					tmpact->print();
					model_print(">> The write causing the bug:\n");
					nextWrite->print();
					model_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
				}
				if(!execution->has_asserted() && model->getParams()->verifierPluginMode > 1){
					execution->set_assert();
				}
				FATAL(execution, wrt, wrt, "UpdateEndrageAfterWrite: Begin range %u is not compatable with new end range %u\n", range->getBeginRange(), nextWrite->get_seq_number()-1);
			}
			if(range->minMergeEndgeRange(nextWrite->get_seq_number()-1)) {
				setActionIndex(endRangeLastAction.get(execution), i, nextWrite);
			}
		}
	}
}

void PMVerifier::printRangeVector(ModelExecution *execution) {
	ModelVector<Range*> *ranges = getOrCreateRangeVector(execution);
	model_print("Execution: %p\n", execution);
	for (unsigned int i = 0;i < execution->get_num_threads();i++) {
		thread_id_t tid = int_to_id(i);
		Range *range = getOrCreateRange(ranges, tid);
		model_print("%u. ", tid);
		range->print();
	}
	model_print("###########################\n");
}

ModelVector<Range*> * PMVerifier::getOrCreateRangeVector(ModelExecution * exec) {
	ModelVector<Range*> *rangeVector = rangeMap.get(exec);
	if(rangeVector == NULL) {
		rangeVector = new ModelVector<Range*>();
		rangeMap.put(exec, rangeVector);
	}
	if(!beginRangeLastAction.get(exec)) {
		beginRangeLastAction.put(exec, new ModelVector<ModelAction*>());
	}
	if(!endRangeLastAction.get(exec)) {
		endRangeLastAction.put(exec, new ModelVector<ModelAction*>());
	}
	if( rangeVector->size() < exec->get_num_threads()) {
		rangeVector->resize(exec->get_num_threads());
	}
	return rangeVector;
}

Range * PMVerifier::getOrCreateRange(ModelVector<Range*> *ranges, int tid) {
	ASSERT(ranges->size() > (uint) tid);
	if(!(*ranges)[tid]) {
		(*ranges)[tid] = new Range();
	}
	return (*ranges)[tid];
}

ModelAction * PMVerifier::getActionIndex(ModelVector<ModelAction*> *actions, unsigned int index) {
	if(index < actions->size()) {
		return (*actions)[index];
	} else {
		return NULL;
	}
}

void PMVerifier::setActionIndex(ModelVector<ModelAction*> *actions, unsigned int index, ModelAction *action) {
	if( index >= actions->size()) {
		actions->resize(index + 1);
	}
	(*actions)[index] = action;
}

void PMVerifier::ignoreAnalysisForLocation(char * addrs, size_t size) {
	auto mp = new ModelPair<char*, size_t>(addrs, size);
	modelPairList.push_back(mp);
	for (uintptr_t ptr = getCacheID(addrs);ptr < (uintptr_t)addrs+size;ptr+=CACHELINESIZE) {
		auto ignoreList = ignoreTable.get(ptr);
		if(!ignoreList) {
			ignoreList = new ModelVector<ModelPair<char*, size_t>*>();
			ignoreTable.put(ptr, ignoreList);
		}
		ignoreList->push_back(mp);
	}
}

bool PMVerifier::ignoreVariable(void * address) {
	uintptr_t ptr = getCacheID(address);
	auto ignoreList = ignoreTable.get(ptr);
	if(ignoreList) {
		for(uint i=0;i< ignoreList->size();i++) {
			auto mp = (*ignoreList)[i];
			uintptr_t lowerlimit = (uintptr_t)mp->p1;
			uintptr_t upperlimit = (uintptr_t)mp->p1 + mp->p2;
			if(upperlimit > (uintptr_t)address && lowerlimit >= (uintptr_t)address) {
				return true;
			}
		}
	}
	return false;
}

bool PMVerifier::checkBeginRangeInversion(ModelExecution *exec, ModelAction *action) {
	ModelVector<Range*> *rangeVector = getOrCreateRangeVector(exec);
	for(uint i=0;i< exec->get_num_threads();i++) {
		Range * range = getOrCreateRange(rangeVector, i);
		ClockVector * cv = action->get_cv();
		ASSERT(cv);
		thread_id_t tid = int_to_id(i);
		if(range->getEndRange()<cv->getClock(tid)) {
			if(model->getParams()->pmdebug > 0 ) {
				model_print("~~~~~~~~~ FATAL RANGE INVERSION ERROR (precheck)~~~~~~~~~~~~\n");
				range->print();
				model_print(">> Range Begin Action:\n");
				ModelAction *tmpact = getActionIndex(beginRangeLastAction.get(exec), i);
				if(tmpact) {
					tmpact->print();
				} else {
					model_print("No ModelAction Yet\n");
				}
				model_print(">> End Range Action:\n");
				tmpact = getActionIndex(endRangeLastAction.get(exec), i);
				tmpact->print();
				model_print(">> The write causing the bug:\n");
				action->print();
				model_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}
			return true;
		}
	}
	return false;
}

bool PMVerifier::checkEndRangeInversion(ModelExecution *execution, ModelAction *wrt, uintptr_t curraddress) {
	// Update the endRange for all threads
	mllnode<ModelAction *> * nextNode = wrt->getActionRef()->getNext();
	ModelVector<ModelAction*> nextWrites;
	findFirstWriteInEachThread(nextWrites, nextNode, curraddress, execution->get_num_threads());
	ModelVector<Range*> *ranges = getOrCreateRangeVector(execution);
	for(uint i=0;i < nextWrites.size();i++) {
		ModelAction *nextWrite = nextWrites[i];
		if(nextWrite) {
			Range *range = getOrCreateRange(ranges, i);
			if(range->getBeginRange()>nextWrite->get_seq_number()-1) {
				if(model->getParams()->pmdebug > 0 ) {
					model_print("~~~~~~~~~ FATAL RANGE INVERSION ERROR IN Setting EndRange (precheck) ~~~~~~~~~~~~\n");
					range->print();
					model_print(">> Range Begin Action:\n");
					ModelAction *tmpact = getActionIndex(beginRangeLastAction.get(execution), i);
					if(tmpact) {
						tmpact->print();
					} else {
						model_print("No ModelAction Yet!\n");
					}
					model_print(">> End Range Action:\n");
					tmpact = getActionIndex(endRangeLastAction.get(execution), i);
					tmpact->print();
					model_print(">> The write causing the bug:\n");
					nextWrite->print();
					model_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
				}
				return true;
			}
		}
	}
	return false;
}
#include "pmverifier.h"
#include "execution.h"
#include "action.h"
#include "model.h"
#include "clockvector.h"
#include "threads-model.h"

PMVerifier::~PMVerifier() {
	rangeMap.resetanddelete();
}

void PMVerifier::freeExecution(ModelExecution *exec) {
	ModelVector<Range*> *rangeVector = rangeMap.get(exec);
	if(rangeVector) {
		for(uint i=0;i< rangeVector->size();i++) {
			delete (*rangeVector)[i];
		}
		rangeVector->clear();
	}
}

/**
 * This simple analysis updates the endRange for all threads when an execution crashes.
 */
void PMVerifier::crashAnalysis(ModelExecution * execution) {
	ModelVector<Range*> *ranges = getOrCreateRangeVector(execution);
	for (unsigned int i = 0;i < execution->get_num_threads();i++) {
		thread_id_t tid = int_to_id(i);
		ModelAction * action = execution->get_last_action(tid);
		if(action) {
			Range *range = getOrCreateRange(ranges, tid);
			range->setEndRange(action->get_seq_number());
		}
	}

}

/**
 * This analysis checks all writes that the execution may possibly read from are in range.
 */
void PMVerifier::mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set) {
	ModelExecution *currExecution = model->get_execution();
	for(uint j=0;j< rf_set->size();j++) {
		SnapVector<Pair<ModelExecution *, ModelAction *> > * writeVec = (*rf_set)[j];
		for(uint i=0;i<read->getOpSize();i++ ) {
			ModelExecution * execution = (*writeVec)[i].p1;
			ModelAction *wrt = (*writeVec)[i].p2;
			if(currExecution != execution ) {
				// Check for persistency bugs
				ModelVector<Range*> *rangeVector = getOrCreateRangeVector(execution);
				Range *range = getOrCreateRange(rangeVector, id_to_int(wrt->get_tid()));
				if(!range->isInRange(wrt->get_seq_number())) {
					ERROR(execution, wrt, read, "Fatal Persistency Bug on Write");
				}
				ASSERT(wrt->get_cv());
				ClockVector *cv = wrt->get_cv();
				for(uint i=0;i< execution->get_num_threads();i++) {
					range = getOrCreateRange(rangeVector, i);
					thread_id_t tid = int_to_id(i);
					if(range->getEndRange() < cv->getClock(tid)) {
						ERROR(execution, wrt, read,
									"Fatal Persistency Bug on read from thread_id= %d\t with clock= %u out of range[%u,%u]\t");
					}
				}
			}
		}
	}
}

void PMVerifier::recordProgress(ModelExecution *exec, ModelAction *action) {
	ModelVector<Range*> *rangeVector = getOrCreateRangeVector(exec);
	for(uint i=0;i< exec->get_num_threads();i++) {
		Range * range = getOrCreateRange(rangeVector, i);
		ClockVector * cv = action->get_cv();
		ASSERT(cv);
		thread_id_t tid = int_to_id(i);
		range->mergeBeginRange(cv->getClock(tid));
	}
}

/**
 * This analysis checks for the conflict and if it found none, updates the beginRange for all threads.
 * If the write is RMW, it finds the first next write in each thread and update their endRanges. Otherwise,
 * if the write was a normal write, it updates the threads' endRanges so they don't read from out of range
 * writes.
 */
void PMVerifier::readFromWriteAnalysis(ModelAction *read, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) {
	ModelExecution *currExecution = model->get_execution();
	void * address = read->get_location();
	for(uint i=0;i<read->getOpSize();i++ ) {
		uintptr_t curraddress = ((uintptr_t)address) + i;
		ModelExecution * execution = (*rfarray)[i].p1;
		if(currExecution != execution) {
			ModelAction *wrt = (*rfarray)[i].p2;
			recordProgress(execution, wrt);
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
			range->minMergeEndgeRange(nextWrite->get_seq_number()-1);
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
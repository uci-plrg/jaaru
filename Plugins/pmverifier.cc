#include "pmverifier.h"
#include "execution.h"
#include "action.h"
#include "model.h"
#include "threads-model.h"

PMVerifier::~PMVerifier() {
    rangeMap.resetanddelete();
}

void PMVerifier::freeExecution(ModelExecution *exec) {
    ModelVector<Range*> *rangeVector = rangeMap.get(exec);
    if(rangeVector) {
        for(uint i=0; i< rangeVector->size(); i++){
            delete (*rangeVector)[i];
        }
    }
}

/**
 * This simple analysis updates the endRange for all threads when an execution crashes.
 */
void PMVerifier::crashAnalysis(ModelExecution * execution) {
    ModelVector<Range*> *ranges = getOrCreateRanges(execution);
    for (unsigned int i = 0;i < execution->get_num_threads();i ++) {
		int tid = id_to_int(i);
		ModelAction * action = execution->getThreadLastAction(tid);
        Range *range = getOrCreateRange(ranges, tid);
        range->setEndRange(action->get_seq_number());
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
                ModelVector<Range*> *rangeVector = getOrCreateRanges(execution);
                Range *range = getOrCreateRange(rangeVector, wrt->get_tid());
                if(!range->isInRange(wrt->get_seq_number())) {
                    FATAL(execution, wrt, read, "Fatal Persistency Bug");
                }
			}
		}
	}
}

/**
 * This analysis checks for the conflict and if it found none, updates the beginRange for all threads.
 * If the write is RMW, it finds the first next write in each thread and update their endRanges. Otherwise,
 * if the write was a normal write, it updates the threads' endRanges so they don't read from out of range
 * writes.
 */
void PMVerifier::readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) {
}

ModelVector<Range*> * PMVerifier::getOrCreateRanges(ModelExecution * exec) {
    ModelVector<Range*> *rangeVector = rangeMap.get(exec);
	if(rangeVector == NULL) {
		rangeVector = new ModelVector<Range*>();
		rangeMap.put(exec, rangeVector);
	} 
    if( rangeVector->size() < exec->get_num_threads()){
        rangeVector->resize(exec->get_num_threads());
    }
    return rangeVector;
}

Range * PMVerifier::getOrCreateRange(ModelVector<Range*> *ranges, int tid) {
    Range *range = (*ranges)[tid];
    if(!range) {
        (*ranges)[tid] = new Range();
    }
    return range;
}
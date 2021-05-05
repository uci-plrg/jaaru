#include "persistrace.h"
#include "model.h"
#include "cacheline.h"
#include "clockvector.h"
#include "action.h"
#include "execution.h"

CacheLineMetaData::CacheLineMetaData(ModelExecution *exec, uintptr_t id) :
	MetaDataKey(exec, id),
	lastFlush(0)
{
}

bool PersistRace::flushExistsBeforeCV(ModelAction *write, ClockVector *cv) {
	ModelVector<ModelAction*> * flushVect = flushmap.get(write);
	if(!flushVect) {
		return false;
	}
	for(uint j=0;j< flushVect->size();j++) {
		ModelAction *existingFlush = (*flushVect)[j];
		if(existingFlush && cv->synchronized_since(existingFlush)) {
			return true;
		}
	}
	return false;
}

void PersistRace::updateFlushVector(ModelAction *write, ModelAction *flush){
	ModelVector<ModelAction*> * flushVect = flushmap.get(write);
	if(!flushVect) {
		flushVect = new ModelVector<ModelAction*>();
		flushmap.put(write, flushVect);
	}
	unsigned int index = flush->get_tid();
	if (index >= flushVect->size())
		flushVect->resize(index + 1);
	(*flushVect)[index] = flush;
}

void CacheLineMetaData::mergeLastFlush(modelclock_t lf){
	if(lf > lastFlush) {
		lastFlush = lf;
	}
}

PersistRace::~PersistRace(){
	auto iter = cachelineMetaSet.iterator();
	while(iter->hasNext()) {
		delete (CacheLineMetaData*)iter->next();
	}
	delete iter;
	beginRangeCV.resetanddelete();
	fullBeginRangeCV.resetanddelete();
	flushmap.resetanddelete();
}

/**
 * This is called when a clflush evicts from store buffer, or a clflushopt is evicted from flush buffer
 * because of existence of RMW, sfence, mfence, locked operations, SC writes. For clflushopt, we wait until
 * the fence operation is executed, then we start processing them.
 */
void PersistRace::evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) {
	ASSERT(flush->is_cache_op() && flush->get_cv());
	if(flush->is_clflush()) {
		num_crash_injection_points++;
		CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, flush);
		ModelAction *prevWrite = NULL;
		for(uint i=0;i< CACHELINESIZE;i++) {
			ModelAction *write = clmetadata->getLastWrites()[i];
			if(write && prevWrite != write && write->happens_before(flush)) {
				if(!flushExistsBeforeCV(write,flush->get_cv())) {
					updateFlushVector(write,flush);
				}
				prevWrite = write;
			}
		}
	} else {
		pendingclwbs.push_back(flush);
	}


}

/**
 * This analysis tracks the most recent write to each address within the cacheline.
 * Note: write operation can write in two cache lines.
 */
void PersistRace::evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action) {
	if(action->is_write()) {
		CacheLineMetaData * clmetadata = getOrCreateCacheLineMeta(execution, action);
		uintptr_t addr = (uintptr_t)action->get_location();
		uintptr_t currCacheID = getCacheID(action->get_location());
		uint wsize = action->getOpSize();
		for(uintptr_t currAddr = addr;currAddr < addr + wsize;currAddr++) {
			if(getCacheID((void*)currAddr) != currCacheID) {
				// Modifying the next cache line
				clmetadata = getOrCreateCacheLineMeta(execution, getCacheID((void*)currAddr));
			}
			uint32_t index = WRITEINDEX(currAddr);
			clmetadata->getLastWrites()[index] = action;
		}
	}
}

/**
 * This analysis checks for persistency race for any writes that 'read' variable can read from is non-atomic, there is a persistency race if:
 *   1) write's seq_number > cacheline's last flush
 *   2) there is no flush in flushmap that happened before BeginRange clock vector of pre-crash execution.
 */
void PersistRace::mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set){
	void * address = read->get_location();
	ModelExecution *currExecution = model->get_execution();
	for(uint j=0;j< rf_set->size();j++) {
		SnapVector<Pair<ModelExecution *, ModelAction *> > * writeVec = (*rf_set)[j];
		for(uint i=0;i<read->getOpSize();i++ ) {
			ModelExecution * execution = (*writeVec)[i].p1;
			ModelAction *wrt = (*writeVec)[i].p2;
			if(currExecution != execution && !wrt->is_atomic_write()) {
				// Check for persistency race
				uintptr_t currAddr = ((uintptr_t)address) + i;
				CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, getCacheID((void*)currAddr));
				ClockVector* brCV = beginRangeCV.get(execution);
				bool flushExist = brCV ? flushExistsBeforeCV(wrt, brCV) : false;
				if(!flushExist && wrt->get_seq_number() > clmetadata->getLastFlush()) {
					ERROR(execution, wrt, read, "Persistency Race");
				}
				brCV = fullBeginRangeCV.get(execution);
				flushExist = brCV ? flushExistsBeforeCV(wrt, brCV) : false;
				if(!flushExist && wrt->get_seq_number() > clmetadata->getLastFlush()) {
					WARNING(execution, wrt, read, "Persistency Race");
				}
			}
		}
	}
}

/**
 * This analysis performs 2 following tasks:
 * (1) If wrt is executed by the current execution, update the BeginRange clock vector for that execution.
 * (2) If wrt is atomic and happended in pre-crash, merge it lastFlush metadata.
 */
void PersistRace::readFromWriteAnalysis(ModelAction *read, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) {
	ModelExecution *currExecution = model->get_execution();
	for(uint i=0;i<read->getOpSize();i++ ) {
		ModelExecution * execution = (*rfarray)[i].p1;
		ModelAction *wrt = (*rfarray)[i].p2;
		CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, wrt);
		if(execution != currExecution) {
			// Reading from pre-crash
			if(wrt->is_atomic_write()) {
				if(clmetadata->getLastFlush() < wrt->get_seq_number()) {
					clmetadata->mergeLastFlush(wrt->get_seq_number());
				}
			}
			// Updating beginRange to record the progress of threads
			persistUntilActionAnalysis(execution, wrt);
			persistUntilActionAnalysis(execution, wrt, false);
		}
	}
}

CacheLineMetaData * PersistRace::getOrCreateCacheLineMeta(ModelExecution * execution, ModelAction *action) {
	return getOrCreateCacheLineMeta(execution, getCacheID(action->get_location()));
}

CacheLineMetaData * PersistRace::getOrCreateCacheLineMeta(ModelExecution * execution, uintptr_t cacheid) {
	MetaDataKey key (execution, cacheid);
	CacheLineMetaData *clmetadata = (CacheLineMetaData *)cachelineMetaSet.get(&key);
	if(clmetadata == NULL) {
		clmetadata = new CacheLineMetaData(execution, cacheid);
		cachelineMetaSet.add(clmetadata);
	}
	return clmetadata;
}

void PersistRace::fenceExecutionAnalysis(ModelExecution *execution, ModelAction *fence) {
	ASSERT(fence->get_cv() != NULL);
	for(uint i=0;i<pendingclwbs.size();i++) {
		ModelAction *clwb = pendingclwbs[i];
		ASSERT(clwb->get_seq_number() < fence->get_seq_number());
		CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, clwb);
		for(uint i=0;i< CACHELINESIZE;i++) {
			ModelAction *write = clmetadata->getLastWrites()[i];
			ModelAction *lastWrite = clwb->getLastWrite() ? clwb->getLastWrite() : clwb;
			if(write && write->happens_before(lastWrite)) {
				if(!flushExistsBeforeCV(write, fence->get_cv())) {
					updateFlushVector(write, fence);
				}
			}
		}
	}
	if(pendingclwbs.size() > 0) {
		num_crash_injection_points++;
		pendingclwbs.clear();
	}
}

void PersistRace::freeExecution(ModelExecution *exec) {
	action_list_t *list = exec->getActionTrace();
	for (mllnode<ModelAction*> *it = list->begin();it != NULL;it=it->getNext()) {
		ModelAction *act = it->getVal();
		if(act->is_cache_op() || act->is_write()) {
			MetaDataKey key (exec, getCacheID(act->get_location()));
			CacheLineMetaData *data = (CacheLineMetaData*) cachelineMetaSet.get(&key);
			if(data) {
				cachelineMetaSet.remove(&key);
				delete data;
			}
			ModelVector<ModelAction*>* flushVect = flushmap.get(act);
			if(flushVect) {
				flushmap.remove(act);
				delete flushVect;
			}
		}
	}
	ClockVector* cv = beginRangeCV.get(exec);
	if(cv) {
		beginRangeCV.remove(exec);
		delete cv;
	}
	cv = fullBeginRangeCV.get(exec);
	if(cv) {
		fullBeginRangeCV.remove(exec);
		delete cv;
	}
}

void PersistRace::persistUntilActionAnalysis(ModelExecution *execution, ModelAction *action, bool prefix) {
	ClockVector* beginRange = prefix ? beginRangeCV.get(execution) : fullBeginRangeCV.get(execution);
	if(beginRange == NULL) {
		beginRange = new ClockVector(NULL, action);
		prefix ? beginRangeCV.put(execution, beginRange) : fullBeginRangeCV.put(execution, beginRange);
	} else {
		if(action->get_cv()) {
			beginRange->merge(action->get_cv());
		}
	}
}

void PersistRace::printStats() {
	model_print("~~~~~~~~~~~~~~~ %s Stats ~~~~~~~~~~~~~~~\n", getName());
	model_print("Total number of prefix-execution bugs: %d\n", num_total_bugs);
	model_print("Number of distinct prefix-execution bugs: %d\n", errorSet.getSize());
	model_print("Total number of full-execution bugs: %d\n", num_total_warnings);
	model_print("Number of distinct full-execution bugs: %d\n", warningSet.getSize());
	model_print("Total number of crash injection points: %d\n", num_crash_injection_points);
}

unsigned int hashCacheLineKey(MetaDataKey *clm) {
	return ((uintptr_t)clm->getExecution() >> 4) ^ ((uintptr_t)clm->getCacheID() >> 6);
}

bool equalCacheLineKey(MetaDataKey *clm1, MetaDataKey *clm2) {
	return (clm1 && clm2 && clm1->getExecution() == clm2->getExecution() && clm1->getCacheID() == clm2->getCacheID()) || (clm1 == clm2 && clm1 == NULL);
}

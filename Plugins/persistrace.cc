#include "persistrace.h"
#include "model.h"
#include "cacheline.h"
#include "clockvector.h"
#include "action.h"
#include "execution.h"

CacheLineMetaData::CacheLineMetaData(ModelExecution *exec, uintptr_t id): 
MetaDataKey(exec, id),
lastFlush(0)
{
}

bool CacheLineMetaData::flushExistsBeforeCV(uint writeIndex, ClockVector *cv) {
    for(uint j=0; j< flushvector[writeIndex].size(); j++) {
        ModelAction *existingFlush = flushvector[writeIndex][j];
        if(existingFlush && cv->synchronized_since(existingFlush)) {
            return true;
        }
    }
    return false;
}

void CacheLineMetaData::updateFlushVector(uint writeIndex, ModelAction *flush){
    unsigned int index = flush->get_tid();
	if (index >= flushvector[writeIndex].size())
		flushvector[writeIndex].resize(index + 1);
    flushvector[writeIndex][index] = flush;
}

void CacheLineMetaData::mergeLastFlush(modelclock_t lf){
    if(lf > lastFlush) {
        lastFlush = lf;
    }
}

PersistRace::~PersistRace(){
    auto iter = cachelineMetaSet.iterator();
    while(iter->hasNext()){
        delete (CacheLineMetaData*)iter->next();
    }
    delete iter;
    beginRangeCV.resetanddelete();
}

/**
 * This is called when a clflush evicts from store buffer, or a clflushopt is evicted from flush buffer
 * because of existence of RMW, sfence, mfence, locked operations, SC writes. For clflushopt, we wait until
 * the fence operation is executed, then we start processing them.
 */
void PersistRace::evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) {
    ASSERT(flush->is_cache_op() && flush->get_cv());
    if(flush->is_clflush()) {
        CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, flush);
        ModelAction *prevWrite = NULL;
        for(uint i=0; i< CACHELINESIZE; i++) {
            ModelAction *write = clmetadata->getLastWrites()[i];
            if(write && prevWrite != write && write->get_seq_number() <= flush->get_seq_number()) {
                if(!clmetadata->flushExistsBeforeCV(i,flush->get_cv())) {
                    clmetadata->updateFlushVector(i,flush);
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
        for(uintptr_t currAddr = addr; currAddr < addr + wsize; currAddr++) {
            if(getCacheID((void*)currAddr) != currCacheID){
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
    for(uint j=0; j< rf_set->size(); j++) {
        SnapVector<Pair<ModelExecution *, ModelAction *> > * writeVec = (*rf_set)[j];
        for(uint i=0; i<read->getOpSize(); i++ ) {
            ModelExecution * execution = (*writeVec)[i].p1;
            ModelAction *wrt = (*writeVec)[i].p2;
            if(currExecution != execution && !wrt->is_atomic_write()){
                // Check for persistency race
                uintptr_t currAddr = ((uintptr_t)address) + i;
                CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, getCacheID((void*)currAddr));
                ClockVector* brCV = beginRangeCV.get(execution);
                bool flushExist = brCV? clmetadata->flushExistsBeforeCV(WRITEINDEX(currAddr), brCV) : false;
                if(!flushExist && wrt->get_seq_number() > clmetadata->getLastFlush()){
                    ERROR(execution, wrt, "Persistency Race");
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
    for(uint i=0; i<read->getOpSize(); i++ ) {
		ModelExecution * execution = (*rfarray)[i].p1;
		ModelAction *wrt = (*rfarray)[i].p2;
        CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, wrt);
        if(execution != currExecution) {
            // Reading from pre-crash
            if(wrt->is_atomic_write()){
                if(clmetadata->getLastFlush() < wrt->get_seq_number()) {
                    clmetadata->mergeLastFlush(wrt->get_seq_number());
                }
            }
        } 
        // Updating beginRange to record the progress of threads
        ClockVector* beginRange = beginRangeCV.get(execution);
        if(beginRange == NULL){
            beginRange = new ClockVector(NULL, wrt);
            beginRangeCV.put(execution, beginRange);
        } else {
            beginRange->merge(wrt->get_cv());
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
    ASSERT(fence->is_fence() && fence->get_cv() != NULL);
    for(uint i=0; i<pendingclwbs.size(); i++) {
        ModelAction *clwb = pendingclwbs[i];
        ASSERT(clwb->get_seq_number() < fence->get_seq_number());
        CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, clwb);
        for(uint i=0; i< CACHELINESIZE; i++) {
            ModelAction *write = clmetadata->getLastWrites()[i];
            modelclock_t lastWriteClk = clwb->getLastWrite()->get_seq_number();
            if(write && write->get_seq_number() <= lastWriteClk) {
                if(!clmetadata->flushExistsBeforeCV(i, fence->get_cv())) {
                    clmetadata->updateFlushVector(i, fence);
                }
            }
        }
    }
    if(pendingclwbs.size() > 0){
        pendingclwbs.clear();
    }
}


unsigned int hashCacheLineKey(MetaDataKey *clm) {
    return ((uintptr_t)clm->getExecution() >> 4) ^ ((uintptr_t)clm->getCacheID() >> 6);
}

bool equalCacheLineKey(MetaDataKey *clm1, MetaDataKey *clm2) {
    return (clm1 && clm2 && clm1->getExecution() == clm2->getExecution() && clm1->getCacheID() == clm2->getCacheID()) || (clm1 == clm2 && clm1 == NULL);
}

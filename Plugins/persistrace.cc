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

bool CacheLineMetaData::flushExistsBeforeFence(modelclock_t flush_seq) {
    for(uint j=0; j< flushvector.size(); j++) {
        ModelAction *existingFlush = flushvector[j];
        //TODO: Need to change it to happen-before not TSO-before
        if(existingFlush && existingFlush->get_seq_number() < flush_seq) {
            return true;
        }
    }
    return false;
}

bool CacheLineMetaData::flushExistsAfterWrite(ModelAction *write){
    for(uint j=0; j< flushvector.size(); j++) {
        ModelAction *existingFlush = flushvector[j];
        if(existingFlush && write->happens_before(existingFlush)){
            return true;
        }
    }
    return false;
}

void CacheLineMetaData::updateFlushVector(ModelAction *flush){
    unsigned int i = flush->get_tid();
	if (i >= flushvector.size())
		flushvector.resize(i + 1);
    flushvector[flush->get_tid()] = flush;
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
 * 
 * 
 */
void PersistRace::evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) {
    modelclock_t flush_seq = flush->get_cv()? flush->get_seq_number(): execution->get_curr_seq_num() + 1;
    CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, flush);
    for(uint i=0; i< CACHELINESIZE; i++) {
        ModelAction *write = clmetadata->getLastWrites()[i];
        if(write && write->get_seq_number() <= flush_seq) {
            bool flushExist = clmetadata->flushExistsAfterWrite(write);
            if(!flushExist && write->happens_before(flush) ) {
                clmetadata->updateFlushVector(flush);
            } else if(flushExist) {
                bool hbfence = clmetadata->flushExistsBeforeFence(flush_seq);
                if(!hbfence) {
                    clmetadata->updateFlushVector(flush);
                }
            }
        }
    }

}

/**
 * 
 * 
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
            uint32_t index = currAddr & (CACHELINESIZE -1);
            clmetadata->getLastWrites()[index] = action;
        }
    }
}

/**
 * 
 * 
 */
void PersistRace::readFromWriteAnalysis(ModelExecution *execution, ModelAction *write) {
    CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, write);
    if(execution != model->get_execution()) {
        // Reading from pre-crash
        if(write->is_rmw()){
            if(clmetadata->getLastFlush() < write->get_seq_number()) {
                clmetadata->setLastFlush(write->get_seq_number());
            }
        } else {
            // Check for persistency race

        }
    }
    // Updating beginRange to record the progress of threads
    ClockVector* beginRange = beginRangeCV.get(execution);
    if(beginRange == NULL){
        beginRange = new ClockVector(NULL, write);
        beginRangeCV.put(execution, beginRange);
    }
    beginRange->merge(write->get_cv());
}

CacheLineMetaData * PersistRace::getOrCreateCacheLineMeta(ModelExecution * execution, ModelAction *action) {
    return getOrCreateCacheLineMeta(execution, getCacheID(action->get_location()));
}

CacheLineMetaData * PersistRace::getOrCreateCacheLineMeta(ModelExecution * execution, uintptr_t cacheid) {
    MetaDataKey meta {execution, cacheid};
    CacheLineMetaData *clmetadata = (CacheLineMetaData *)cachelineMetaSet.get(&meta);
    if(clmetadata == NULL) {
        clmetadata = new CacheLineMetaData(execution, cacheid);
        cachelineMetaSet.add(clmetadata);
    }
    return clmetadata;
}


unsigned int hashCacheLineKey(MetaDataKey *clm) {
    return ((uintptr_t)clm->getExecution() >> 4) ^ ((uintptr_t)clm->getCacheID() >> 6);
}

bool equalCacheLineKey(MetaDataKey *clm1, MetaDataKey *clm2) {
    return (clm1 && clm2 && clm1->getExecution() == clm2->getExecution() && clm1->getCacheID() == clm2->getCacheID()) || (clm1 == clm2 && clm1 == NULL);
}

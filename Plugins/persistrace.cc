#include "persistrace.h"
#include "model.h"
#include "cacheline.h"
#include "clockvector.h"
#include "action.h"
#include "execution.h"

CacheLineMetaData::CacheLineMetaData(ModelExecution *exec, ModelAction *action):
execution(exec),
cacheID(getCacheID(action->get_location())),
lastFlush(0)
{
}

CacheLineMetaData::CacheLineMetaData(ModelExecution *exec, uintptr_t cid):
execution(exec),
cacheID(cid),
lastFlush(0)
{
}

/**
 * 
 * 
 */
void PersistRace::evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) {
    modelclock_t flush_seq = flush->get_cv()? flush->get_seq_number(): execution->get_curr_seq_num() + 1;
    CacheLineMetaData *clmetadata = getOrCreateCacheLineMeta(execution, flush);
    for(uint i=0; i< CACHELINESIZE; i++) {
        ModelAction *write = clmetadata->lastWrites[i];
        if(write && write->get_seq_number() <= flush_seq) {
            bool flushExist = false;
            for(uint j=0; j< clmetadata->flushvector.size(); j++) {
                ModelAction *existingFlush = clmetadata->flushvector[j];
                if(existingFlush && write->happens_before(existingFlush)){
                    flushExist = true;
                    break;
                }
            }
            if(!flushExist && write->happens_before(flush) ) {
                clmetadata->flushvector[flush->get_tid()] = flush;
            } else if(flushExist) {
                bool happenBefore = false;
                for(uint j=0; j< clmetadata->flushvector.size(); j++) {
                    ModelAction *existingFlush = clmetadata->flushvector[j];
                    //TODO: Need to change it to happen-before not TSO-before
                    if(existingFlush && existingFlush->get_seq_number() < flush_seq) {
                        happenBefore = true;
                        break;
                    }
                }
                if(!happenBefore) {
                    clmetadata->flushvector[flush->get_tid()] = flush;
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
        uint wsize = action->getOpSize();
        for(uintptr_t currAddr = addr; currAddr < addr + wsize; currAddr++) {
            if(getCacheID((void*)currAddr) != clmetadata->cacheID){
                // Modifying the next cache line
                clmetadata = getOrCreateCacheLineMeta(execution, getCacheID((void*)currAddr));
            }
            uint32_t index = currAddr & (CACHELINESIZE -1);
            clmetadata->lastWrites[index] = action;
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
            if(clmetadata->lastFlush < write->get_seq_number()) {
                clmetadata->lastFlush = write->get_seq_number();
            }
        } else {
            // Check for persistency race
        }
    }
    // Updating beginRange to record the progress of threads
    ClockVector* beginRange = beginRangeCV.get(execution);
    if(beginRange == NULL){
        beginRange = new ClockVector(NULL, write);
    }
    beginRange->merge(write->get_cv());
}

CacheLineMetaData * PersistRace::getOrCreateCacheLineMeta(ModelExecution * execution, ModelAction *action) {
    CacheLineMetaData meta(execution, action);
    CacheLineMetaData *clmetadata = cachelineMetaSet.get(&meta);
    if(clmetadata == NULL) {
        clmetadata = new CacheLineMetaData(execution, action);
        cachelineMetaSet.add(clmetadata);
    }
    return clmetadata;
}


unsigned int hashCacheLineKey(CacheLineMetaData *clm) {
    return ((uintptr_t)clm->execution >> 4) ^ ((uintptr_t)clm->cacheID >> 6);
}

bool equalCacheLineKey(CacheLineMetaData *clm1, CacheLineMetaData *clm2) {
    return (clm1 && clm2 && clm1->execution == clm2->execution && clm1->cacheID == clm2->cacheID) || (clm1 == clm2 && clm1 == NULL);
}

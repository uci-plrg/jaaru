#ifndef PERSISTRACE_H
#define PERSISTRACE_H
#include "classlist.h"
#include "analysis.h"
#include "cacheline.h"

class CacheLineMetaData {
public:
    CacheLineMetaData(ModelExecution* , ModelAction *);
    CacheLineMetaData(ModelExecution* , uintptr_t);
    ModelExecution *execution;
    uintptr_t cacheID;
    modelclock_t lastFlush;
    ModelVector<ModelAction*> flushvector;
    ModelAction *lastWrites [CACHELINESIZE]= {NULL};
};

unsigned int hashCacheLineMeta(CacheLineMetaData *);
bool equalCacheLineMeta(CacheLineMetaData *, CacheLineMetaData *);

class PersistRace: public Analysis {
public:
    PersistRace() {}
    void crashAnalysis(ModelExecution * execution){}
    void mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set){}
    const char * getName() {return PERSISTRACENAME;}
    void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush);
    void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action);
    void readFromWriteAnalysis(ModelExecution *execution, ModelAction *write);
private:
    CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, ModelAction *action);
    CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, uintptr_t cid);

    HashSet<CacheLineMetaData *, uintptr_t, 0, model_malloc, model_calloc, model_free, hashCacheLineMeta, equalCacheLineMeta> cachelineMetaSet;
    HashTable<ModelExecution*, ClockVector*, uintptr_t, 2, model_malloc, model_calloc, model_free> beginRangeCV;

};

#endif
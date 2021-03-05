#ifndef PERSISTRACE_H
#define PERSISTRACE_H
#include "classlist.h"
#include "analysis.h"
#include "cacheline.h"

class MetaDataKey {
public:
    MetaDataKey(ModelExecution *exec, uintptr_t id): execution(exec), cacheID(id) {}
    ModelExecution *getExecution() {return execution;}
    uintptr_t getCacheID() {return cacheID;}
protected:
    ModelExecution *execution;
    uintptr_t cacheID;
};

class CacheLineMetaData: public MetaDataKey {
public:
    CacheLineMetaData(ModelExecution *exec, uintptr_t id);
    ModelVector<ModelAction*> * getFlushVector() { return &flushvector;}
    modelclock_t getLastFlush() {return lastFlush;}
    void mergeLastFlush(modelclock_t lf);
    ModelAction **getLastWrites() {return lastWrites;}
    bool flushExistsBeforeCV(ClockVector *cv);
    void updateFlushVector(ModelAction *flush);
private:
    modelclock_t lastFlush;
    ModelVector<ModelAction*> flushvector;
    ModelAction *lastWrites [CACHELINESIZE]= {NULL};
};

unsigned int hashCacheLineMeta(MetaDataKey *);
bool equalCacheLineMeta(MetaDataKey *, MetaDataKey *);

class PersistRace: public Analysis {
public:
    PersistRace() {}
    ~PersistRace();
    void crashAnalysis(ModelExecution * execution){}
    void mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set){}
    const char * getName() {return PERSISTRACENAME;}
    void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush);
    void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action);
    void readFromWriteAnalysis(ModelExecution *execution, ModelAction *write);
private:
    CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, uintptr_t cid);
    CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, ModelAction *action);

    HashSet<MetaDataKey*, uintptr_t, 0, model_malloc, model_calloc, model_free, hashCacheLineMeta, equalCacheLineMeta> cachelineMetaSet;
    HashTable<ModelExecution*, ClockVector*, uintptr_t, 2, model_malloc, model_calloc, model_free> beginRangeCV;

};

#endif
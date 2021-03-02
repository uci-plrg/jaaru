#ifndef PERSISTRACE_H
#define PERSISTRACE_H
#include "classlist.h"
#include "analysis.h"
#include "cacheline.h"

class CacheLineMetaData {
public:
    ModelExecution *execution;
    uintptr_t cacheID;
    modelclock_t lastFlush;
    ModelVector<ModelAction*> flushvector;
    ModelAction *lastWrites [CACHELINESIZE];
};

unsigned int hashCacheLineMeta(CacheLineMetaData *);
bool equalCacheLineMeta(CacheLineMetaData *, CacheLineMetaData *);

class PersistRace: public Analysis {
public:
    void crashAnalysis(ModelExecution * execution){}
    void mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set){}
    void readFromWriteAnalysis(ModelExecution *execution, ModelAction *write);
    const char * getName() {return PERSISTRACENAME;}
private:
    HashSet<CacheLineMetaData *, uintptr_t, 0, model_malloc, model_calloc, model_free, hashCacheLineMeta, equalCacheLineMeta> cachelineMetaSet;
    HashTable<ModelExecution*, ClockVector*, uintptr_t, 2, model_malloc, model_calloc, model_free> beginRangeCV;

};

#endif
#ifndef PERSISTRACE_H
#define PERSISTRACE_H
#include "classlist.h"
#include "analysis.h"
#include "cacheline.h"

#define WRITEINDEX(addr) (addr & (CACHELINESIZE -1))

class MetaDataKey {
public:
	MetaDataKey(ModelExecution *exec, uintptr_t id) : execution(exec), cacheID(id) {}
	ModelExecution *getExecution() {return execution;}
	uintptr_t getCacheID() {return cacheID;}
	MEMALLOC

protected:
	ModelExecution *execution;
	uintptr_t cacheID;
};

class CacheLineMetaData : public MetaDataKey {
public:
	CacheLineMetaData(ModelExecution *exec, uintptr_t id);
	modelclock_t getLastFlush() {return lastFlush;}
	void mergeLastFlush(modelclock_t lf);
	ModelAction **getLastWrites() {return lastWrites;}
private:
	modelclock_t lastFlush;
	ModelAction *lastWrites [CACHELINESIZE]= {NULL};
};

unsigned int hashCacheLineKey(MetaDataKey *);
bool equalCacheLineKey(MetaDataKey *, MetaDataKey *);

class PersistRace : public Analysis {
public:
	PersistRace() {}
	~PersistRace();
	void crashAnalysis(ModelExecution * execution);
	void mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set);
	const char * getName() {return PERSISTRACENAME;}
	void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush);
	void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action);
	void readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray);
	void fenceExecutionAnalysis(ModelExecution *execution, ModelAction *action);
	void freeExecution(ModelExecution *exec);
	void persistExecutionAnalysis(ModelExecution *execution);
	void enterRecoveryProcedure() {}
	void exitRecoveryProcedure() {}
	void ignoreAnalysisForLocation(char * addrs, size_t size) {}
	void printStats();
private:
	CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, uintptr_t cid);
	CacheLineMetaData * getOrCreateCacheLineMeta(ModelExecution *, ModelAction *action);
	bool flushExistsBeforeCV(ModelAction *write, ClockVector *cv);
	void updateFlushVector(ModelAction *write, ModelAction *flush);
	void persistUntilAction(ModelExecution *exec, ModelAction *act);

	HashSet<MetaDataKey*, uintptr_t, 0, model_malloc, model_calloc, model_free, hashCacheLineKey, equalCacheLineKey> cachelineMetaSet;
	HashTable<ModelExecution*, ClockVector*, uintptr_t, 2, model_malloc, model_calloc, model_free> beginRangeCV;
	HashTable<ModelAction*, ModelVector<ModelAction*>*, uintptr_t, 2, model_malloc, model_calloc, model_free> flushmap;
	ModelVector<ModelAction *> pendingclwbs;
	int num_crash_injection_points = {0};
};

#endif

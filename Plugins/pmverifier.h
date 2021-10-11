#ifndef PMVERIFIER_H
#define PMVERIFIER_H
#include "hashtable.h"
#include "classlist.h"
#include "range.h"
#include "analysis.h"

class PMVerifier : public Analysis {
public:
	PMVerifier() : disabled(false){}
	~PMVerifier();
	void crashAnalysis(ModelExecution * execution);
	void mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set);
	void readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray);
	const char * getName() {return PMVERIFIERNAME;}
	void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush){}
	void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action){}
	void fenceExecutionAnalysis(ModelExecution *execution, ModelAction *action){}
	void freeExecution(ModelExecution *exec);
	void persistExecutionAnalysis(ModelExecution *execution) {}
	void getRegionFromIDAnalysis(ModelExecution *execution, ModelVector<ModelAction*>* thrdLastActions);
	void enterRecoveryProcedure() {disabled = true;}
	void exitRecoveryProcedure() {disabled = false;}
	void ignoreAnalysisForLocation(char * addrs, size_t size);
	void printStats() {}
private:
	bool recordProgress(ModelExecution *exec, ModelAction *action);
	void findFirstWriteInEachThread(ModelVector<ModelAction*> &nextWrites, mllnode<ModelAction *> * node, uintptr_t curraddress, unsigned int numThreads);
	void updateThreadsEndRangeafterWrite(ModelExecution *exec, ModelAction *action, uintptr_t curraddress);
	void crashInnerExecutionsBeforeFirstWrite(ModelExecution *execution, uintptr_t curraddress);
	ModelVector<Range*> *getOrCreateRangeVector(ModelExecution * exec);
	Range * getOrCreateRange(ModelVector<Range*> *ranges,  int tid);
	ModelAction * getActionIndex(ModelVector<ModelAction*> *ranges, unsigned int index);
	void setActionIndex(ModelVector<ModelAction*> *ranges, unsigned int index, ModelAction *action);
	void printRangeVector(ModelExecution *execution);
	ModelAction* populateWriteConstraint(Range &range, ModelAction *wrt, ModelExecution * wrtExecution, uintptr_t curraddress);
	void printWriteAndFirstReadByThread(ModelExecution *exec, modelclock_t wclock, ModelAction *readThreadAction);
	bool ignoreVariable(void * address);
	bool checkBeginRangeInversion(ModelExecution *exec, ModelAction *action);
	bool checkEndRangeInversion(ModelExecution *execution, ModelAction *wrt, uintptr_t curraddress);

	HashTable<ModelExecution*, ModelVector<Range*>*, uintptr_t, 2, model_malloc, model_calloc, model_free> rangeMap;
	HashTable<ModelExecution*, ModelVector<ModelAction*>*, uintptr_t, 2, model_malloc, model_calloc, model_free> beginRangeLastAction;
	HashTable<ModelExecution*, ModelVector<ModelAction*>*, uintptr_t, 2, model_malloc, model_calloc, model_free> endRangeLastAction;
	HashTable<uintptr_t, ModelVector<ModelPair<char*, size_t>*>*, uintptr_t, 2, model_malloc, model_calloc, model_free> ignoreTable;
	ModelVector<ModelPair<char*, size_t>*> modelPairList;
	bool disabled;
};

#endif

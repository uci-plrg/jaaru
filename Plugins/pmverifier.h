#ifndef PMVERIFIER_H
#define PMVERIFIER_H
#include "hashtable.h"
#include "classlist.h"
#include "range.h"
#include "analysis.h"

class PMVerifier : public Analysis {
public:
	PMVerifier(){}
	~PMVerifier(){}
	void crashAnalysis(ModelExecution * execution);
	void mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set);
	void readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray);
	const char * getName() {return PMVERIFIERNAME;}
	void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush){}
	void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action){}
	void fenceExecutionAnalysis(ModelExecution *execution, ModelAction *action){}
	void freeExecution(ModelExecution *exec) {}
private:
	HashTable<ModelExecution*, ModelVector<Range *>, uintptr_t, 2, model_malloc, model_calloc, model_free> rangeMap;
};

#endif
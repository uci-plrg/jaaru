#ifndef ANALYSIS_H
#define ANALYSIS_H
#include "classlist.h"
#include "mymemory.h"

#define PMVERIFIERNAME "PM-Verfier"
#define PERSISTRACENAME "PersistRace"

unsigned int hashErrorPosition(char *pos);
bool equalErrorPosition(char *p1, char *p2);

class Analysis {
public:
	virtual const char * getName() = 0;
	virtual void crashAnalysis(ModelExecution * execution) = 0;
	virtual void mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *>* rf_set) = 0;
	virtual void readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) = 0;
	virtual void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) = 0;
	virtual void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action) = 0;
	virtual void fenceExecutionAnalysis(ModelExecution *execution, ModelAction *action) = 0;
	virtual void freeExecution(ModelExecution *exec) = 0;
	virtual void persistExecutionAnalysis(ModelExecution *execution) = 0;
	virtual void printStats() = 0;
	void ERROR(ModelExecution *exec, ModelAction * write,  ModelAction *read, const char * message);
	void WARNING(ModelExecution *exec, ModelAction * write,  ModelAction *read, const char * message);
	void FATAL(ModelExecution *exec, ModelAction *write, ModelAction *read, const char * message, ...);
	MEMALLOC
protected:
	HashSet<char*, uintptr_t, 0, model_malloc, model_calloc, model_free, hashErrorPosition, equalErrorPosition> errorSet;
	HashSet<char*, uintptr_t, 0, model_malloc, model_calloc, model_free, hashErrorPosition, equalErrorPosition> warningSet;
	int num_total_bugs = {0};
	int num_total_warnings = {0};
};

#endif

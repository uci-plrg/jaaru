#ifndef ANALYSIS_H
#define ANALYSIS_H
#include "classlist.h"

#define PMVERIFIERNAME "PM-Verfier"
#define PERSISTRACENAME "PersistRace"

unsigned int hashErrorPosition(ModelAction *a);
bool equalErrorPosition(ModelAction *a1, ModelAction *a2);

class Analysis {
public:
    virtual const char * getName() = 0;
    virtual void crashAnalysis(ModelExecution * execution) = 0;
    virtual void mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *>* rf_set) = 0;
    virtual void readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) = 0;
    virtual void evictFlushBufferAnalysis(ModelExecution *execution, ModelAction *flush) = 0;
    virtual void evictStoreBufferAnalysis(ModelExecution *execution, ModelAction *action) = 0;
    virtual void fenceExecutionAnalysis(ModelExecution *execution, ModelAction *action) = 0;
    void ERROR(ModelAction * action, const char * message);
protected:
    HashSet<ModelAction*, uintptr_t, 0, model_malloc, model_calloc, model_free, hashErrorPosition, equalErrorPosition> errorSet;
};

#endif
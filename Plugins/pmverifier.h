#ifndef PMVERIFIER_H
#define PMVERIFIER_H
#include "hashtable.h"
#include "classlist.h"
#include "range.h"
#include "analysis.h"

class PMVerifier: public Analysis {
public:
    PMVerifier();
    ~PMVerifier();
    void crashAnalysis(ModelExecution * execution);
    void mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set);
    void readFromWriteAnalysis(ModelExecution *execution, ModelAction *write);
    const char * getName() {return PMVERIFIERNAME;}
private:
    HashTable<ModelExecution*, ModelVector<Range *>, uintptr_t, 2, model_malloc, model_calloc, model_free> rangeMap;
};

#endif
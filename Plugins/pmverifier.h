#ifndef PMVERIFIER_H
#define PMVERIFIER_H
#include "hashtable.h"
#include "classlist.h"
#include "range.h"

class PMVerifier {
public:
    PMVerifier();
    ~PMVerifier();
    void crash(ModelExecution * execution);
    bool mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set);
    bool readFromWriteAnalysis(ModelExecution *execution, ModelAction *write);
private:
    HashTable<ModelExecution*, ModelVector<Range *>, uintptr_t, 2, model_malloc, model_calloc, model_free> rangeMap;
};

#endif
#ifndef ANALYSIS_H
#define ANALYSIS_H
#include "classlist.h"

#define PMVERIFIERNAME "PM-Verfier"

class Analysis {
public:
    virtual const char * getName() = 0;
    virtual void crashAnalysis(ModelExecution * execution) = 0;
    virtual bool mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set) = 0;
    virtual bool readFromWriteAnalysis(ModelExecution *execution, ModelAction *write) = 0;
};

#endif
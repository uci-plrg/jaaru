#ifndef ANALYSIS_H
#define ANALYSIS_H
#include "classlist.h"

#define PMVERIFIERNAME "PM-Verfier"
#define PERSISTRACENAME "PersistRace"

class Analysis {
public:
    virtual const char * getName() = 0;
    virtual void crashAnalysis(ModelExecution * execution) = 0;
    virtual void mayReadFromAnalysis(SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> rf_set) = 0;
    virtual void readFromWriteAnalysis(ModelExecution *execution, ModelAction *write) = 0;
};

#endif
/**
 * File: varrange.h
 * This folder contains ranges for each variable
 * */

#ifndef VARRANGE_H
#define VARRANGE_H
#include "classlist.h"
#define RANGEUNDEFINED (modelclock_t)-1

class VarRange{
public:
    VarRange(void * address);
    void * getVarAddress() {return address;}
	modelclock_t getBeginRange(){ return beginR;}
	modelclock_t getEndRange(){ return endR;}
	void setBeginRange(modelclock_t begin) { beginR =begin;}
	void setEndRange (modelclock_t end) { endR = end; }
    void applyWrite(ModelAction *write);
    void persistUntil(modelclock_t opclock);

    SNAPSHOTALLOC;
private:
    //Only contains the clock time of 1) the write that a read may read from 2) read clock
	modelclock_t beginR;
	// It should the clock of the last CLWB, when there is a RMW or Fence
	modelclock_t endR;
    // Variable address
    void *address;
};

#endif
/**
 * File: cacheline.h
 * Contains the info that we need for cache line
 *
 */

#ifndef CACHELINE_H
#define CACHELINE_H
#include "mymemory.h"
#include "action.h"
#include "stl-model.h"

#define CACHELINESIZE 64

class CacheLine{
public:
	CacheLine(void *address);
	CacheLine(void *address, modelclock_t beginR);
	uintptr_t getId() {return id;}
	modelclock_t getBeginRange(){ return beginR;}
	modelclock_t getEndRange(){ return endR;}
	void setBeginRange(modelclock_t begin) { beginR =begin;}
	void setEndRange (modelclock_t end) { endR = end; }

	SNAPSHOTALLOC;
private:
	modelclock_t beginR;
	modelclock_t endR;

	uintptr_t id;

};

#endif

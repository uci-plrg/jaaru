/**
 * File: cacheline.h
 * Contains the info that we need for cache line
 *
 */

#ifndef CACHELINE_H
#define CACHELINE_H
#include "mymemory.h"
#include "classlist.h"
#include "stl-model.h"

#define CACHELINESIZE 64

class CacheLine {
public:
	CacheLine(void *address);
	CacheLine(uintptr_t id);
	uintptr_t getId() { return id; }
	void applyWrite(ModelAction *write);
	void persistCacheLine();
	void persistUntil(modelclock_t opclock);
	modelclock_t getBeginRange(){ return beginR;}
	modelclock_t getEndRange(){ return endR;}
	modelclock_t getPersistentSeqNumber() const;
	void setLastCacheOp(ModelAction *clop) {lastCacheOp = clop;}
	void setBeginRange(modelclock_t begin) { beginR =begin;}
	void setEndRange (modelclock_t end) { endR = end; }

	SNAPSHOTALLOC;
private:
	
	uintptr_t id;
	//Only contains the clock time of 1) the write that a read may read from 2) read clock
	modelclock_t beginR;
	// It should the clock of the last CLWB, when there is a RMW or Fence
	modelclock_t endR;
	ModelAction *lastCacheOp;
};

inline uintptr_t getCacheID(void *address){
	//DEBUG( "Address %p === Cache ID %x\n", address, ((uintptr_t)address) & ~(CACHELINESIZE - 1) );
	return ((uintptr_t)address) & ~(CACHELINESIZE - 1);
}

#endif

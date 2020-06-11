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
	uintptr_t getId() { return id; }
	void setLastCacheOp(ModelAction *cacheop) { lastCacheOp = cacheop; }
	ModelAction *getLastCacheOp() { return lastCacheOp; }
	void applyWrite(ModelAction *write);
	void persistCacheLine();
	void persistUntil(modelclock_t opclock);
	VarRange* getVariable(void *address);
	SNAPSHOTALLOC;
private:
	
	VarRangeSet varSet;
	uintptr_t id;
	//Last cache operation that read from store buffer for this cache line
	ModelAction *lastCacheOp;
};

inline uintptr_t getCacheID(void *address){
	//DEBUG( "Address %p === Cache ID %x\n", address, ((uintptr_t)address) & ~(CACHELINESIZE - 1) );
	return ((uintptr_t)address) & ~(CACHELINESIZE - 1);
}

#endif

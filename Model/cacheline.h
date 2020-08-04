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
	modelclock_t getBeginRange(){ return beginR; }
	modelclock_t getEndRange(){ return endR; }
	ModelAction * getLastCacheOp() { return lastCacheOp; }
	void setLastCacheOp(ModelAction *clop) { lastCacheOp = clop; }
	ModelAction * getLastWrite() { return lastWrite; }
	void setLastWrite(ModelAction *clwr) { lastWrite = clwr; }
	void setBeginRange(modelclock_t begin) { beginR = begin; }
	void setEndRange(modelclock_t end) { endR = end; }

	MEMALLOC;
private:
	uintptr_t id;
	ModelAction *lastWrite;
	ModelAction *lastCacheOp;

	//earliest point at which the cache line could have been written
	//out...  Incluside, so this includes write with beginR sequence
	//number
	modelclock_t beginR;
	//last point at which the cache line could have been
	//persisted. Inclusive, so this includes write with endR sequence
	//number
	modelclock_t endR;
};

inline uintptr_t getCacheID(void *address) {
	return ((uintptr_t)address) & ~(CACHELINESIZE - 1);
}
#endif

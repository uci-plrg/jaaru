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
#include "range.h"
#define CACHELINESIZE 64

class CacheLine : public Range {
public:
	CacheLine(void *address);
	CacheLine(uintptr_t id);
	uintptr_t getId() { return id; }
	ModelAction * getLastCacheOp() { return lastCacheOp; }
	void setLastCacheOp(ModelAction *clop) { lastCacheOp = clop; }
	ModelAction * getLastWrite() { return lastWrite; }
	void setLastWrite(ModelAction *clwr) { lastWrite = clwr; }

	MEMALLOC;
private:
	uintptr_t id;
	ModelAction *lastWrite;
	ModelAction *lastCacheOp;

	//beginR: earliest point at which the cache line could have been written
	//out...  Incluside, so this includes write with beginR sequence
	//number

	//endR: last point at which the cache line could have been
	//persisted. Inclusive, so this includes write with endR sequence
	//number
};

inline uintptr_t getCacheID(const void *address) {
	return ((uintptr_t)address) & ~(CACHELINESIZE - 1);
}
#endif

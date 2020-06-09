/**
 * File: threadmemory.h
 * This is the shadow memory for simulating TSO memory semantics for X86 processor.
 * This memory tracks the effects of memory operations on the memory to validate
 * the program crash consistency property.
 */

#ifndef THREADMEMORY_H
#define THREADMEMORY_H
#include "stl-model.h"
#include "hashset.h"
#include "action.h"


class ThreadMemory {
public:
	ThreadMemory();
	void addWrite(ModelAction * write);
	void applyRead(ModelAction *read);
	void addCacheOp(ModelAction *clflush);
	void applyFence(ModelAction *fence);
	void applyRMW(ModelAction *write);
	modelclock_t getCacheLineBeginRange(void * location);
	SNAPSHOTALLOC;
private:
	void executeWrite(ModelAction *write);
	void executeCacheOp(ModelAction *read);
	void emptyStoreBuffer();
	void persistMemoryBuffer();
	void persistCacheLine(ModelAction *cacheOp);

	SnapVector<ModelAction*> storeBuffer;
	CacheLineSet cache;
	MemoryBufferSet memoryBuffer;
};

#endif

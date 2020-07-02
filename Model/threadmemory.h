/**
 * File: threadmemory.h
 * This is the shadow memory for simulating TSO memory semantics for X86 processor.
 * This memory tracks the effects of memory operations on the memory to validate
 * the program crash consistency property.
 */

#ifndef THREADMEMORY_H
#define THREADMEMORY_H
#include "stl-model.h"
#include "hashtable.h"
#include "action.h"
#include "actionlist.h"

class ThreadMemory {
public:
	ThreadMemory();
	void addWrite(ModelAction * write);
	void getWritesFromStoreBuffer(void *address, SnapVector<ModelAction *> * rf_set);
	ModelAction * getLastWriteFromSoreBuffer(void *address);
	void addCacheOp(ModelAction *clflush);
	void applyFence(ModelAction *fence);
	void persistUntil(modelclock_t opclock);
	SnapList<CacheLine*>* getCacheLines(void * address) {return obj_to_cachelines.get(address);}
	void writeToCacheLine(ModelAction *write);

	SNAPSHOTALLOC;
private:
	void evictWrite(ModelAction *write);
	void executeWriteOperation(ModelAction *write);
	void executeCacheOp(ModelAction *read);
	void persistCacheLine(CacheLine *cl);
	void emptyStoreBuffer();
	void persistMemoryBuffer();
	
	HashTable<const void *, SnapList<CacheLine*> *, uintptr_t, 2> obj_to_cachelines;
	CacheLineSet activeCacheLines;
	SnapList<ModelAction*> storeBuffer;
	CacheLineSet memoryBuffer;
};

#endif

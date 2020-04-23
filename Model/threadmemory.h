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


class ThreadMemory{
public:
	ThreadMemory();
	void applyWrite(ModelAction * write, uint size);
	void applyRead(ModelAction *read, uint size);
	void applyCacheOp(ModelAction *clflush);
	void applyFence(ModelAction *fence);
	void applyRMW(ModelAction *write);

	SNAPSHOTALLOC;
private:
	SnapVector<ModelAction*> storeBuffer;
	CacheLineSet cache;
	ValueSet memoryBuffer;
};

ThreadMemory* getThreadMemory();

#endif

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
	ModelAction * getLastWriteFromStoreBuffer(ModelAction *read);
	void addCacheOp(ModelAction *act);
	void addOp(ModelAction *act);
	void addWrite(ModelAction *act);
	ModelAction *popFromStoreBuffer();
	void writeToCacheLine(ModelAction *write);
	void emptyStoreBuffer();
	void emptyFlushBuffer();
	void emptyWrites(void * address, uint size);

	SNAPSHOTALLOC;
private:
	void evictOpFromStoreBuffer(ModelAction *op);
	void evictWrite(ModelAction *write);
	void evictNonAtomicWrite(ModelAction *na_write);
	void evictFlushOpt(ModelAction *flushopt);

	SnapList<ModelAction*> storeBuffer;
	HashTable<uintptr_t, ModelAction *, uintptr_t, 6> obj_to_last_write;
	SnapList<ModelAction *> flushBuffer;
	ModelAction * lastclflush;
};
#endif

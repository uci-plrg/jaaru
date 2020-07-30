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

inline bool checkOverlap(ModelExecution *exec, SnapVector<Pair<ModelExecution *, ModelAction *> >*writes, ModelAction *write, uint & numslotsleft, uintptr_t rbot, uintptr_t rtop, uint size) {
	uintptr_t wbot = (uintptr_t) write->get_location();
	uint wsize = write->getOpSize();
	uintptr_t wtop = wbot + wsize;
	//skip on if there is no overlap
	if ((wbot >= rtop) || (rbot >= wtop))
		return false;
	intptr_t writeoffset = ((intptr_t)rbot) - ((intptr_t)wbot);
	for(uint i = 0;i < size;i++) {
		if ((*writes)[i].p2 == NULL && writeoffset >= 0 && writeoffset < wsize) {
			(*writes)[i].p1 = exec;
			(*writes)[i].p2 = write;
			numslotsleft--;
			if (numslotsleft == 0)
				return true;
		}
		writeoffset++;
	}
	return false;
}

class ThreadMemory {
public:
	ThreadMemory();
	bool getLastWriteFromStoreBuffer(ModelAction *read, ModelExecution *exec, SnapVector<Pair<ModelExecution *, ModelAction *> >*writes, uint & numslotsleft);
	void addCacheOp(ModelAction *act);
	void addOp(ModelAction *act);
	void addWrite(ModelAction *act);
	ModelAction *popFromStoreBuffer();
	void writeToCacheLine(ModelAction *write);
	void emptyStoreBuffer();
	void emptyFlushBuffer();
	void emptyWrites(void * address);
	bool hasPendingFlushes();

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
	int flushcount = 0;
};
#endif

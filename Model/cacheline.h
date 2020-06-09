/**
 * File: cacheline.h
 * Contains the info that we need for cache line
 *
 */

#ifndef CACHELINE_H
#define CACHELINE_H
#include <cmath>
#include "mymemory.h"
#include "action.h"
#include "stl-model.h"

#define CACHELINESIZE 64

class CacheLine {
public:
	CacheLine(void *address);
	CacheLine(uintptr_t _id);
	uintptr_t getId() {return id;}
	modelclock_t getBeginRange(){ return beginR;}
	modelclock_t getEndRange(){ return endR;}
	void setBeginRange(modelclock_t begin) { beginR =begin;}
	void setEndRange (modelclock_t end) { endR = end; }
	void applyWrite(ModelAction *write);
	bool nonpersistentWriteExist();
	SNAPSHOTALLOC;
private:
	//Only contains the clock time of 1) the write that a read may read from 2) read clock
	modelclock_t beginR;
	// It should the clock of the FENCE or RMW operation.
	modelclock_t endR;

	uintptr_t id;

};

inline uintptr_t getCacheID(void *address){
	ASSERT( ((uintptr_t)address & 0x3) == 0 );
	int bitShift = static_cast<uintptr_t>(log2(static_cast<double>(CACHELINESIZE) ) );
	DEBUG( "Address %p === Cache ID %x", address, ((uintptr_t)address >> bitShift) );
	return ((uintptr_t)address) >>  bitShift;
}

#endif

#include "threadmemory.h"
#include "common.h"

ThreadMemory::ThreadMemory() :
	storeBuffer(),
	cache(),
	memoryBuffer
{
}

void applyWrite(ModelAction * write) {
	storeBuffer.push_back(write);
}

void applyRead(ModelAction *read) {
	ASSERT(0);
}

void applyCacheOp(ModelAction *clflush){
	storeBuffer.push_back(clflush);
}

void applyFence(ModelAction *fence){
	ASSERT(0);
}

void applyRMW(ModelAction *write){
	ASSERT(0);
}


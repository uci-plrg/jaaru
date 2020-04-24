#include "threadmemory.h"
#include "common.h"

ThreadMemory::ThreadMemory() :
	storeBuffer(),
	cache(),
	memoryBuffer()
{
}

void ThreadMemory::applyWrite(ModelAction * write) {
	storeBuffer.push_back(write);
}

void ThreadMemory::applyRead(ModelAction *read) {
	ASSERT(0);
}

void ThreadMemory::applyCacheOp(ModelAction *clflush){
	storeBuffer.push_back(clflush);
}

void ThreadMemory::applyFence(ModelAction *fence){
	ASSERT(0);
}

void ThreadMemory::applyRMW(ModelAction *write){
	ASSERT(0);
}


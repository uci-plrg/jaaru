#include "threadmemory.h"
#include "cacheline.h"
#include "common.h"
#include "model.h"

ThreadMemory::ThreadMemory() :
	storeBuffer(),
	cache(),
	memoryBuffer()
{
}

void ThreadMemory::applyWrite(ModelAction * write) 
{
	storeBuffer.push_back(write);
}

void ThreadMemory::applyRead(ModelAction *read) 
{
	emptyStoreBuffer();
	ASSERT(0);
}

void ThreadMemory::applyCacheOp(ModelAction *clflush)
{
	storeBuffer.push_back(clflush);
}

void ThreadMemory::applyFence(ModelAction *fence)
{
	emptyStoreBuffer();
	emptyMemoryBuffer(fence);
	ASSERT(0);
}

void ThreadMemory::applyRMW(ModelAction *rmw)
{
	emptyStoreBuffer();
	emptyMemoryBuffer(rmw);
	ASSERT(0);
}

void ThreadMemory::emptyStoreBuffer()
{
	uint bsize = storeBuffer.size();
	for( uint i=0; i < bsize; i++) {
		storeBufferDequeue();
	}
}

void ThreadMemory::emptyMemoryBuffer(ModelAction *op)
{
	ValueSetIter *iter = memoryBuffer.iterator();
	while(iter->hasNext()){
		uintptr_t clid = iter->next();
		persistCacheLine(clid, op);
	}
	delete iter;
}

void ThreadMemory::storeBufferDequeue()
{
	ModelAction *curr = storeBuffer.back(); 
	storeBuffer.pop_back();
	if (curr->is_write()){
		executeWrite(curr);
	} else if (curr->is_cache_op()){
		executeCacheOp(curr);
	} else {
		//There is an operation other write, memory fence, and cache operation in the store buffer!!
		ASSERT(0);
	}

}


void ThreadMemory::executeWrite(ModelAction *writeop)
{
	DEBUG("Executing write size %u to memory location %p", writeop->getOperatorSize(), writeop->get_location());
	CacheLine ctmp(writeop->get_location());
	CacheLine *cline = cache.get(&ctmp);
	if (cline == NULL) { // There is no write for this cache line
		cline = new CacheLine(writeop->get_location());
		cache.add(cline);
	}
	cline->applyWrite(writeop);
}

void ThreadMemory::executeCacheOp(ModelAction *cacheop)
{
	uintptr_t cid = getCacheID(cacheop->get_location());
	if(cacheop->is_clflush()){
		persistCacheLine(cid, cacheop);
	} else {
		memoryBuffer.add(cid);
	}
}

void ThreadMemory::persistCacheLine(uintptr_t cid, ModelAction *op){
	memoryBuffer.remove(cid);
	CacheLine tmp(cid);
	CacheLine *cline = cache.get(&tmp);
	ASSERT(cline != NULL);
	cline->setEndRange(op->get_seq_number());
}


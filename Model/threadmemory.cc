#include "threadmemory.h"
#include "cacheline.h"
#include "common.h"
#include "model.h"
#include "execution.h"

ThreadMemory::ThreadMemory() :
	storeBuffer(),
	cache(),
	memoryBuffer()
{
}

void ThreadMemory::addWrite(ModelAction * write)
{
	storeBuffer.push_back(write);
}

void ThreadMemory::applyRead(ModelAction *read)
{
	DEBUG("Executing read size %u to memory location %p", read->getOperatorSize(), read->get_location());
	emptyStoreBuffer();
	//TODO: decide about value that might read.
	ASSERT(0);
}

void ThreadMemory::addCacheOp(ModelAction *clflush)
{
	storeBuffer.push_back(clflush);
}

void ThreadMemory::applyFence(ModelAction *fence)
{
	DEBUG("Applying memory fence...");
	emptyStoreBuffer();
	persistMemoryBuffer();
	ASSERT(0);
}

void ThreadMemory::applyRMW(ModelAction *rmw)
{
	DEBUG("Executing read-modify-write size %u to memory location %p", rmw->getOperatorSize(), rmw->get_location());
	emptyStoreBuffer();
	persistMemoryBuffer();
	//TODO: decide about read and writing the new value ...
	ASSERT(0);
}

void ThreadMemory::emptyStoreBuffer()
{
	while(storeBuffer.size() > 0) {
		ModelAction *curr = storeBuffer.back();
		storeBuffer.pop_back();
		if (curr->is_write()) {
			executeWrite(curr);
		} else if (curr->is_cache_op()) {
			executeCacheOp(curr);
		} else {
			//There is an operation other write, memory fence, and cache operation in the store buffer!!
			ASSERT(0);
		}
	}
}

void ThreadMemory::executeWrite(ModelAction *writeop)
{
	DEBUG("Executing write size %u to memory location %p", writeop->getOperatorSize(), writeop->get_location());
	CacheLine ctmp(writeop->get_location());
	CacheLine *cline = cache.get(&ctmp);
	if (cline == NULL) {	// There is no write for this cache line
		cline = new CacheLine(writeop->get_location());
		cache.add(cline);
	}
	cline->applyWrite(writeop);
}

void ThreadMemory::executeCacheOp(ModelAction *cacheop)
{
	if(cacheop->is_clflush()) {
		persistCacheLine(cacheop);
		memoryBuffer.remove(cacheop);
	} else {
		if(memoryBuffer.contains(cacheop)) {
			model->get_execution()->add_warning("Warning: Redundant cache opeartion for the cache id = %x", getCacheID(cacheop->get_location()) );
			memoryBuffer.remove(cacheop);
		}
		memoryBuffer.add(cacheop);
	}
}

void ThreadMemory::persistMemoryBuffer()
{
	MemoryBufferSetIter *iter = memoryBuffer.iterator();
	while(iter->hasNext()) {
		ModelAction *cacheOp = iter->next();
		persistCacheLine(cacheOp);
	}
	delete iter;
}

void ThreadMemory::persistCacheLine(ModelAction *cacheOp){
	ASSERT(cacheOp->is_cache_op());
	CacheLine tmp(cacheOp->get_location());
	CacheLine *cline = cache.get(&tmp);
	ASSERT(cline);
	ModelAction *prevWrite = model->get_execution()->get_last_write_before(cacheOp);
	ASSERT(prevWrite);
	cline->setEndRange(prevWrite->get_seq_number());
}

modelclock_t ThreadMemory::getCacheLineBeginRange(void * location)
{
	CacheLine tmp(location);
	CacheLine *cline = cache.get(&tmp);
	ASSERT(cline);
	return cline->getBeginRange();
}
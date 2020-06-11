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
/**
 * Getting all the stores to the location from the store buffer.
 * */
void ThreadMemory::getWritesFromStoreBuffer(void *address, SnapVector<ModelAction *> * rf_set)
{
	//DEBUG("Executing read size %u to memory location %p\n", read->getOperatorSize(), read->get_location());
	for(uint i=0; i< storeBuffer.size(); i++){
		ModelAction *write = storeBuffer[i];
		if(write->is_write() && write->get_location() == address){
			rf_set->push_back(write);
		}
	}
}

void ThreadMemory::addCacheOp(ModelAction *clflush)
{
	storeBuffer.push_back(clflush);
}

void ThreadMemory::applyFence(ModelAction *fence)
{
	//DEBUG("Applying memory fence...\n");
	emptyStoreBuffer();
	persistMemoryBuffer();
}

void ThreadMemory::applyRMW(ModelAction *rmw)
{
	//DEBUG("Executing read-modify-write size %u to memory location %p\n", rmw->getOperatorSize(), rmw->get_location());
	emptyStoreBuffer();
	persistMemoryBuffer();
	//TODO: decide about read and writing the new value ...
	ASSERT(0);
}

void ThreadMemory::emptyStoreBuffer()
{
	for(uint i=0; i < storeBuffer.size(); i++) {
		ModelAction *curr = storeBuffer[i];
		if (curr->is_write()) {
			executeWrite(curr);
		} else if (curr->is_cache_op()) {
			executeCacheOp(curr);
		} else {
			//There is an operation other write, memory fence, and cache operation in the store buffer!!
			ASSERT(0);
		}
	}
	storeBuffer.clear();
}

void ThreadMemory::executeWrite(ModelAction *writeop)
{
	//DEBUG("Executing write size %u W[%p] = %" PRIu64 "\n", writeop->getOperatorSize(), writeop->get_location(), writeop->get_value());
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
	//DEBUG("Executing cache operation for address %p (Cache ID = %u)\n", cacheop->get_location(), getCacheID(cacheop->get_location()));
	CacheLine tmp(cacheop->get_location());
	CacheLine *cacheline = cache.get(&tmp);
	if(cacheline == NULL){
		model->get_execution()->add_warning("Warning: Redundant cache opeartion for unmodified cache id = %x\n", getCacheID(cacheop->get_location()) );
		return;
	}
	cacheline->setLastCacheOp(cacheop);
	if(cacheop->is_clflush()) {
		cacheline->persistCacheLine();
		memoryBuffer.remove(cacheline);
	} else {
		if(memoryBuffer.contains(cacheline)) {
			model->get_execution()->add_warning("Warning: Redundant cache opeartion for the cache id = %x\n", getCacheID(cacheop->get_location()) );
		} else {
			memoryBuffer.add(cacheline);
		}
	}
}

void ThreadMemory::persistMemoryBuffer()
{
	CacheLineSetIter *iter = memoryBuffer.iterator();
	while(iter->hasNext()) {
		CacheLine *cacheline = iter->next();
		cacheline->persistCacheLine();
	}
	memoryBuffer.reset();
	delete iter;
}

VarRange* ThreadMemory::getVarRange(void * writeAddress)
{
	ASSERT(writeAddress);
	CacheLine tmp(writeAddress);
	CacheLine *cline = cache.get(&tmp);
	if(cline == NULL){
		return NULL;
	}
	return cline->getVariable(writeAddress);
}

void ThreadMemory::persistUntil(modelclock_t opclock)
{
	CacheLineSetIter *iter = cache.iterator();
	while(iter->hasNext()){
		CacheLine *line = iter->next();
		line->persistUntil(opclock);
	}
	delete iter;
}
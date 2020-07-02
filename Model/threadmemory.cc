#include "threadmemory.h"
#include "cacheline.h"
#include "common.h"
#include "model.h"
#include "execution.h"
#include "datarace.h"

#define APPLYWRITE(size, obj, val) \
	*((volatile uint ## size ## _t *)obj) = val;

ThreadMemory::ThreadMemory() :
	obj_to_cachelines(),
	activeCacheLines(),
	storeBuffer(),
	memoryBuffer()
{
}

void ThreadMemory::addWrite(ModelAction * write)
{
	storeBuffer.push_back(write);
}

ModelAction * ThreadMemory::getLastWriteFromSoreBuffer(void *address)
{
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end(); rit != NULL; rit=rit->getPrev()) {
		ModelAction *write = rit->getVal();
		if(write->is_write() && write->get_location() == address){
			return write;
		}
	}
	return NULL;
}

/**
 * Getting all the stores to the location from the store buffer.
 * */
void ThreadMemory::getWritesFromStoreBuffer(void *address, SnapVector<ModelAction *> * rf_set)
{
	//DEBUG("Executing read size %u to memory location %p\n", read->getOperatorSize(), read->get_location());
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end(); rit != NULL; rit=rit->getPrev()) {
		ModelAction *write = rit->getVal();
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

void ThreadMemory::emptyStoreBuffer()
{
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.begin(); rit != NULL; rit=rit->getNext()) {
		ModelAction *curr = rit->getVal();
		curr->print();
		if (curr->is_write()) {
			evictWrite(curr);
		} else if (curr->is_cache_op()) {
			executeCacheOp(curr);
		} else {
			//There is an operation other write, memory fence, and cache operation in the store buffer!!
			ASSERT(0);
		}
	}
	storeBuffer.clear();
}

/**
 * executing the actual RMW operation. Then adding checks for datarace.
 * @param rmw a rmw operation
 */
void ThreadMemory::writeToCacheLine(ModelAction *writeop)
{
	CacheLine ctmp(writeop->get_location());
	CacheLine* activeCline = activeCacheLines.get(&ctmp);
	if(activeCline == NULL){ //This cacheline is being touched for the first time.
		activeCline = new CacheLine(writeop->get_location());
		activeCacheLines.add(activeCline);
	}
	SnapList<CacheLine*>* clines = obj_to_cachelines.get(writeop->get_location());
	if(clines == NULL){
		clines = new SnapList<CacheLine*>();
		obj_to_cachelines.put(writeop->get_location(), clines);
	}
	if( clines->size() == 0 || clines->back() != activeCline){
		// This cache line is modified by writing to other variables.
		clines->push_back(activeCline);
	}
	activeCline->applyWrite(writeop);
}

void ThreadMemory::executeWriteOperation(ModelAction *_write)
{
	switch(_write->getOpSize()){
		case 8: APPLYWRITE(8, _write->get_location(), _write->get_value()); break;
		case 16: APPLYWRITE(16, _write->get_location(), _write->get_value()); break;
		case 32: APPLYWRITE(32, _write->get_location(), _write->get_value()); break;
		case 64: APPLYWRITE(64, _write->get_location(), _write->get_value()); break; 
		default:
			model_print("Unsupported write size\n");
			ASSERT(0);
	}
	
	thread_id_t tid = _write->get_tid();
	for(int i=0;i < _write->getOpSize() / 8;i++) {
		atomraceCheckWrite(tid, (void *)(((char *)_write->get_location())+i));
	}
}

void ThreadMemory::evictWrite(ModelAction *writeop)
{
	//Initializing the sequence number
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(writeop);
	//DEBUG("Executing write size %u W[%p] = %" PRIu64 "\n", writeop->getOperatorSize(), writeop->get_location(), writeop->get_value());
	writeToCacheLine(writeop);
	execution->add_write_to_lists(writeop);
	executeWriteOperation(writeop);
	for(int i=0;i < writeop->getOpSize() / 8;i++) {
		atomraceCheckWrite(writeop->get_tid(), (void *)(((char *)writeop->get_location())+i));
	} 
}

void ThreadMemory::executeCacheOp(ModelAction *cacheop)
{
	//DEBUG("Executing cache operation for address %p (Cache ID = %u)\n", cacheop->get_location(), getCacheID(cacheop->get_location()));
	
	model->get_execution()->remove_action_from_store_buffer(cacheop);
	CacheLine tmp(cacheop->get_location());
	CacheLine *cacheline = activeCacheLines.get(&tmp);
	if(cacheline == NULL){
		model->get_execution()->add_warning("Warning: Redundant cache opeartion for unmodified cache id = %x\n", getCacheID(cacheop->get_location()) );
		return;
	}
	cacheline->setLastCacheOp(cacheop);
	if(cacheop->is_clflush()) {
		persistCacheLine(cacheline);
		memoryBuffer.remove(cacheline);
	} else {
		if(memoryBuffer.contains(cacheline)) {
			model->get_execution()->add_warning("Warning: Redundant cache opeartion for the cache id = %x\n", getCacheID(cacheop->get_location()) );
		} else {
			memoryBuffer.add(cacheline);
		}
	}
}

void ThreadMemory::persistCacheLine(CacheLine *cacheline)
{
	ASSERT(activeCacheLines.remove(cacheline));
	cacheline->persistCacheLine();
	CacheLine * newCL = new CacheLine(cacheline->getId());
	newCL->setBeginRange(cacheline->getEndRange());
	activeCacheLines.add(newCL);
}

void ThreadMemory::persistMemoryBuffer()
{
	CacheLineSetIter *iter = memoryBuffer.iterator();
	while(iter->hasNext()) {
		CacheLine *cacheline = iter->next();
		persistCacheLine(cacheline);
	}
	memoryBuffer.reset();
	delete iter;
}


void ThreadMemory::persistUntil(modelclock_t opclock)
{
	CacheLineSetIter *iter = activeCacheLines.iterator();
	while(iter->hasNext()){
		CacheLine *cl = iter->next();
		cl->persistUntil(opclock);
	}
	delete iter;
}

#include "threadmemory.h"
#include "cacheline.h"
#include "common.h"
#include "model.h"
#include "execution.h"
#include "datarace.h"

#define APPLYWRITE(size, obj, val) \
	*((volatile uint ## size ## _t *)obj) = val;

ThreadMemory::ThreadMemory() :
	obj_to_cacheline(),
	storeBuffer(),
	memoryBuffer()
{
}

void ThreadMemory::addWrite(ModelAction * write)
{
	storeBuffer.push_back(write);
}

ModelAction * ThreadMemory::getLastWriteFromStoreBuffer(void *address)
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
	DEBUG("Store Buffer Size = %u, Address requested %p\n", storeBuffer.size(), address);
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end(); rit != NULL; rit=rit->getPrev()) {
		ModelAction *write = rit->getVal();
		if(write->is_write() && write->get_location() == address){
			rf_set->push_back(write);
		}
	}
}

void ThreadMemory::addCacheOp(ModelAction *clflush) {
	storeBuffer.push_back(clflush);
}

void ThreadMemory::applyFence() {
	emptyStoreBuffer();
	persistMemoryBuffer();
}

void ThreadMemory::evictOpFromStoreBuffer(ModelAction *act) {
	ASSERT(act->is_write() || act->is_cache_op());
	if(act->is_nonatomic_write()) {
		evictNonAtomicWrite(act);
	} else if (act->is_write()) {
		evictWrite(act);
	} else if (act->is_cache_op()) {
		evictCacheOp(act);
	} else {
		//There is an operation other write, memory fence, and cache operation in the store buffer!!
		ASSERT(0);
	}
}

ModelAction *ThreadMemory::popFromStoreBuffer() {
	ASSERT(storeBuffer.size() > 0);
	ModelAction *head = storeBuffer.front();
	ASSERT(head != NULL);
	storeBuffer.pop_front();
	head->print();
	evictOpFromStoreBuffer(head);
	return head;
}

void ThreadMemory::emptyStoreBuffer() {
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.begin(); rit != NULL; rit=rit->getNext()) {
		ModelAction *curr = rit->getVal();
		curr->print();
		evictOpFromStoreBuffer(curr);
	}
	storeBuffer.clear();
}

void ThreadMemory::executeWriteOperation(ModelAction *_write) {
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

void ThreadMemory::evictNonAtomicWrite(ModelAction *na_write) {
	executeWriteOperation(na_write);
	switch(na_write->getOpSize()){
		case 8: raceCheckWrite8(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
		case 16: raceCheckWrite16(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
		case 32: raceCheckWrite32(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
		case 64: raceCheckWrite64(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
		default:
			model_print("Unsupported write size\n");
			ASSERT(0);
	}
	delete na_write;
}

void ThreadMemory::evictWrite(ModelAction *writeop)
{
	//Initializing the sequence number
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(writeop);

	execution->add_write_to_lists(writeop);
	executeWriteOperation(writeop);
	for(int i=0;i < writeop->getOpSize() / 8;i++) {
		atomraceCheckWrite(writeop->get_tid(), (void *)(((char *)writeop->get_location())+i));
	} 
}

void ThreadMemory::evictCacheOp(ModelAction *cacheop) {
	model->get_execution()->remove_action_from_store_buffer(cacheop);
  void * loc = cacheop->get_location();
  CacheLine *cacheline = obj_to_cacheline.get(loc);
	if(cacheline == NULL){
    cacheline = new CacheLine(loc);
		obj_to_cacheline.put(loc, cacheline);
	}
	cacheline->setLastCacheOp(cacheop);
	if(cacheop->is_clflush()) {
		persistCacheLine(cacheline);
		memoryBuffer.remove(cacheline);
	} else {
    memoryBuffer.add(cacheline);
	}
}

void ThreadMemory::persistCacheLine(CacheLine *cacheline) {
	cacheline->setBeginRange(cacheline->getLastCacheOp()->get_seq_number());
  cacheline->setLastCacheOp(NULL);
}

void ThreadMemory::persistMemoryBuffer() {
	CacheLineSetIter *iter = memoryBuffer.iterator();
	while(iter->hasNext()) {
		CacheLine *cacheline = iter->next();
		persistCacheLine(cacheline);
	}
	memoryBuffer.reset();
	delete iter;
}

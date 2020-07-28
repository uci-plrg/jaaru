#include "threadmemory.h"
#include "cacheline.h"
#include "common.h"
#include "model.h"
#include "execution.h"
#include "datarace.h"

ThreadMemory::ThreadMemory() :
	storeBuffer(),
	obj_to_last_write(),
	flushBuffer(),
	lastclflush(NULL) {
}

/** Adds CFLUSH or CFLUSHOPT to the store buffer. */

void ThreadMemory::addCacheOp(ModelAction * act) {
	storeBuffer.push_back(act);
	ModelAction *lastWrite = obj_to_last_write.get(getCacheID(act->get_location()));
	act->setLastWrites(model->get_execution()->get_curr_seq_num(), lastWrite);
}

void ThreadMemory::addOp(ModelAction * act) {
	storeBuffer.push_back(act);
}

void ThreadMemory::addWrite(ModelAction * write) {
	storeBuffer.push_back(write);
	obj_to_last_write.put(getCacheID(write->get_location()), write);
}

ModelAction * ThreadMemory::getLastWriteFromStoreBuffer(ModelAction *read) {
	void *address = read->get_location();
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end();rit != NULL;rit=rit->getPrev()) {
		ModelAction *write = rit->getVal();
		if(write->is_write() && write->get_location() == address) {
			return write;
		}
	}
	return NULL;
}

void ThreadMemory::evictOpFromStoreBuffer(ModelAction *act) {
	ASSERT(act->is_write() || act->is_cache_op() || act->is_sfence());
	if(act->is_nonatomic_write()) {
		evictNonAtomicWrite(act);
	} else if (act->is_write()) {
		evictWrite(act);
	} else if (act->is_sfence()) {
		emptyFlushBuffer();
	} else if (act->is_cache_op()) {
		if (act->is_clflush()) {
			lastclflush = act;
			model->get_execution()->evictCacheOp(act);
		} else {
			evictFlushOpt(act);
		}
	} else {
		//There is an operation other write, memory fence, and cache operation in the store buffer!!
		ASSERT(0);
	}
}

void ThreadMemory::evictFlushOpt(ModelAction *act) {
	if (lastclflush != NULL)
		act->set_last_clflush(lastclflush->get_seq_number());
	flushBuffer.push_back(act);
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
	for (rit = storeBuffer.begin();rit != NULL;rit=rit->getNext()) {
		ModelAction *curr = rit->getVal();
		evictOpFromStoreBuffer(curr);
	}
	storeBuffer.clear();
}

void ThreadMemory::emptyFlushBuffer() {
	sllnode<ModelAction *> * rit;
	for (rit = flushBuffer.begin();rit != NULL;rit=rit->getNext()) {
		ModelAction *curr = rit->getVal();
		model->get_execution()->evictCacheOp(curr);
	}
	flushBuffer.clear();
}

void ThreadMemory::evictNonAtomicWrite(ModelAction *na_write) {
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(na_write);
	execution->add_write_to_lists(na_write);
	switch(na_write->getOpSize()) {
	case 8: raceCheckWrite8(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
	case 16: raceCheckWrite16(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
	case 32: raceCheckWrite32(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
	case 64: raceCheckWrite64(na_write->get_tid(), (void *)(((uintptr_t)na_write->get_location()))); break;
	default:
		model_print("Unsupported write size\n");
		ASSERT(0);
	}
}

void ThreadMemory::evictWrite(ModelAction *writeop)
{
	//Initializing the sequence number
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(writeop);

	execution->add_write_to_lists(writeop);
	for(int i=0;i < writeop->getOpSize() / 8;i++) {
		atomraceCheckWrite(writeop->get_tid(), (void *)(((char *)writeop->get_location())+i));
	}
}

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
	lastclflush(NULL),
	flushcount(0) {
}

/** Adds CFLUSH or CFLUSHOPT to the store buffer. */

void ThreadMemory::addCacheOp(ModelAction * act) {
	storeBuffer.push_back(act);
	ModelAction *lastWrite = obj_to_last_write.get(getCacheID(act->get_location()));
	act->setLastWrites(model->get_execution()->get_curr_seq_num(), lastWrite);
	model->get_execution()->updateStoreBuffer(1);
	flushcount++;
}

void ThreadMemory::addOp(ModelAction * act) {
	storeBuffer.push_back(act);
	model->get_execution()->updateStoreBuffer(1);
}

void ThreadMemory::addWrite(ModelAction * write) {
	storeBuffer.push_back(write);
	model->get_execution()->updateStoreBuffer(1);
	obj_to_last_write.put(getCacheID(write->get_location()), write);
}

bool ThreadMemory::getLastWriteFromStoreBuffer(ModelAction *read, ModelExecution * exec, SnapVector<Pair<ModelExecution *, ModelAction *> >*writes, uint & numslotsleft) {
	uint size = read->getOpSize();
	uintptr_t rbot = (uintptr_t) read->get_location();
	uintptr_t rtop = rbot + size;

	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end();rit != NULL;rit=rit->getPrev()) {
		ModelAction *write = rit->getVal();
		if(write->is_write())
			if (checkOverlap(exec, writes, write, numslotsleft, rbot, rtop, size))
				return true;
	}
	return false;
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
			flushcount--;
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

void ThreadMemory::popFromStoreBuffer() {
	if (storeBuffer.size() > 0) {
		ModelAction *head = storeBuffer.front();
		storeBuffer.pop_front();
		model->get_execution()->updateStoreBuffer(-1);
		evictOpFromStoreBuffer(head);
	}
}

void ThreadMemory::emptyStoreBuffer() {
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.begin();rit != NULL;rit=rit->getNext()) {
		ModelAction *curr = rit->getVal();
		evictOpFromStoreBuffer(curr);
	}
	model->get_execution()->updateStoreBuffer(-storeBuffer.size());
	storeBuffer.clear();
}

void ThreadMemory::emptyFlushBuffer() {
	sllnode<ModelAction *> * it;
	for (it = flushBuffer.begin();it != NULL;it=it->getNext()) {
		ModelAction *curr = it->getVal();
		model->get_execution()->evictCacheOp(curr);
		flushcount--;
	}
	flushBuffer.clear();
}

bool ThreadMemory::emptyWrites(void * address) {
	sllnode<ModelAction *> * rit;
	for (rit = storeBuffer.end();rit != NULL;rit=rit->getPrev()) {
		ModelAction *curr = rit->getVal();
		if (curr->is_write()) {
			void *loc = curr->get_location();
			if (((uintptr_t)address) >= ((uintptr_t)loc) && ((uintptr_t)address) < (((uintptr_t)loc)+(curr->getOpSize()))) {
				break;
			}
		}
	}
	if (rit != NULL) {
		sllnode<ModelAction *> * it;
		for(it = storeBuffer.begin();it!= NULL;) {
			sllnode<ModelAction *> *next = it->getNext();
			ModelAction *curr = it->getVal();
			evictOpFromStoreBuffer(curr);
			storeBuffer.erase(it);
			model->get_execution()->updateStoreBuffer(-1);
			if (it == rit)
				return true;
			it = next;
		}
	}
  return false;
}

bool ThreadMemory::hasPendingFlushes() {
	return flushcount != 0;
}

void ThreadMemory::evictNonAtomicWrite(ModelAction *na_write) {
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(na_write);
	execution->add_write_to_lists(na_write);
}

void ThreadMemory::evictWrite(ModelAction *writeop)
{
	//Initializing the sequence number
	ModelExecution *execution = model->get_execution();
	execution->remove_action_from_store_buffer(writeop);

	execution->add_write_to_lists(writeop);
}

#include "pmcheckapi.h"
#include "model.h"
#include "threadmemory.h"
#include "threads-model.h"
#include "common.h"
#include "datarace.h"

// PMC Non-Atomic Store
ThreadMemory* getThreadMemory(){
	createModelIfNotExist();
	return thread_current()->getMemory();

}

#define PMCHECKSTORE(size)                                                                      \
	void pmc_store ## size (void *addrs){                                                   \
		DEBUG("pmc_store%u:addr = %p\n", size, addrs);                                                          \
		ModelAction *action = new ModelAction(NONATOMIC_WRITE, memory_order_seq_cst, addrs);                    \
		action->setOperatorSize(size);                                                                          \
		getThreadMemory()->applyWrite(action);                                                                  \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i< size/8;i++) {                                                                           \
			raceCheckWrite(tid, (void *)(((uintptr_t)addrs) + i));                                          \
		}                                                                                                       \
	}
PMCHECKSTORE(8)
PMCHECKSTORE(16)
PMCHECKSTORE(32)
PMCHECKSTORE(64)


// PMC Non-Atomic Load

#define PMCHECKLOAD(size)                                                                                       \
	void pmc_load ## size (void *addrs) {                                                                   \
		DEBUG("pmc_load%u:addr = %p\n", size, addrs);                                                   \
		ModelAction *action = new ModelAction(NONATOMIC_READ, memory_order_seq_cst, addrs);             \
		action->setOperatorSize(size);                                                                  \
		getThreadMemory()->applyRead( action);                                                          \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i< size/8;i++) {                                                                           \
			raceCheckRead(tid, (void *)(((uintptr_t)addrs) + i));                                           \
		}                                                                                               \
	}

PMCHECKLOAD(8)
PMCHECKLOAD(16)
PMCHECKLOAD(32)
PMCHECKLOAD(64)

// PMC NVM operations



void pmc_clwb(void * addrs){
	DEBUG("pmc_clwb:addr = %p\n",addrs);
	getThreadMemory()->applyCacheOp(new ModelAction(ACTION_CLWB, memory_order_seq_cst, addrs) );
}

void pmc_clflushopt(void * addrs){
	DEBUG("pmc_clflushopt:addr = %p\n",addrs);
	getThreadMemory()->applyCacheOp(new ModelAction(ACTION_CLFLUSHOPT, memory_order_seq_cst, addrs) );
}

void pmc_clflush(void * addrs){
	DEBUG("pmc_clflush:addr = %p\n",addrs);
	getThreadMemory()->applyCacheOp(new ModelAction(ACTION_CLFLUSH, memory_order_seq_cst, addrs) );
}

void pmc_mfence(){
	DEBUG("pmc_mfence\n");
	getThreadMemory()->applyFence(new ModelAction(CACHE_MFENCE) );
}

void pmc_lfence(){
	DEBUG("pmc_lfence\n");
	ASSERT(0);
}

void pmc_sfence(){
	DEBUG("pmc_sfence\n");
	getThreadMemory()->applyFence(new ModelAction(CACHE_SFENCE) );
}


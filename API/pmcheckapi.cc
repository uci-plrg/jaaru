#include "pmcheckapi.h"
#include "model.h"
#include "threadmem.h"
#include "threads-model.h"
#include "common.h"
// PMC Non-Atomic Store
inline ThreadMemory* getThreadMemory(){
	createmodelIfNotExist();
	return = thread_current()->getMemory();
	
}

#define PMCHECKSTORE(size) 									\
	void pmc_store ## size (void *addrs){							\
		DEBUG("pmc_store%u:addr = %p\n", size, addrs);\
		getThreadMemory()->applyWrite(new ModelAction(NONATOMIC_WRITE, memory_order_seq_cst, addrs) ); \
	}
PMCHECKSTORE(8)
PMCHECKSTORE(16)
PMCHECKSTORE(32)
PMCHECKSTORE(64)


// PMC Non-Atomic Load

#define PMCHECKLOAD(size)										\
	void pmc_load ## size (void *addrs) {								\
		DEBUG("pmc_load%u:addr = %p\n", size, addrs);\
		getThreadMemory()->applyRead( new ModelAction(NONATOMIC_READ, memory_order_seq_cst, addrs);	\
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
	getThreadMemory()->applyCacheOp(new ModelAction(ACTION_CLFLUSHOpt, memory_order_seq_cst, addrs) );
}

void pmc_clflush(void * addrs){
	DEBUG("pmc_clflush:addr = %p\n",addrs);
	getThreadMemory()->applyCacheOp(new ModelAction(ACTION_CLFLUSH, memory_order_seq_cst, addrs) );
}

void pmc_mfence(){
	DEBUG("pmc_mfence\n");
	getThreadMemory()->applyFence(new ModelAction(ACTION_CACHEMFENCE) );
}

void pmc_lfence(){
	ASSERT(0);
}

void pmc_sfence(){
	DEBUG("pmc_sfence\n");
	getThreadMemory()->applyFence(new ModelAction(ACTION_CACHESFENCE)
}


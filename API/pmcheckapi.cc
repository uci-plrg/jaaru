#include "pmcheckapi.h"
#include "model.h"
#include "threads-model.h"
#include "common.h"
#include "datarace.h"
#include "action.h"
#include "threadmemory.h"

#define PMCHECKSTORE(size)                                                                                      \
	void pmc_store ## size (void *addrs, uint ## size ## _t val){                                                   \
		DEBUG("pmc_store%u:addr = %p\n", size, addrs);                                                                  \
		if(!model)                                                                                                                                                                              \
			return;                                                                                                                                                                         \
		Thread *thread = thread_current();                                                                                              \
		ModelAction * act = new ModelAction(NONATOMIC_WRITE, memory_order_relaxed, addrs, val, thread, size);   \
		thread->getMemory()->addOp(act);                                                                                                                             \
	}

PMCHECKSTORE(8)
PMCHECKSTORE(16)
PMCHECKSTORE(32)
PMCHECKSTORE(64)


// PMC Non-Atomic Load

#define PMCHECKLOAD(size)                                                                               \
	void pmc_load ## size (void *addrs) {                                                               \
		DEBUG("pmc_load%u:addr = %p\n", size, addrs);                                                   \
		if(!model)                                                                                                                                                                              \
			return;                                                                                                                                                                         \
		thread_id_t tid = thread_current()->get_id();                                                                                           \
		raceCheckRead ## size (tid, (void *)(((uintptr_t)addrs)));                                      \
	}

PMCHECKLOAD(8)
PMCHECKLOAD(16)
PMCHECKLOAD(32)
PMCHECKLOAD(64)

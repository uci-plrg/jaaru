#include "pmcheckapi.h"
#include "model.h"
#include "threads-model.h"
#include "common.h"
#include "datarace.h"
#include "action.h"
#include "threadmemory.h"

#define PMCHECKSTORE(size)                                                                                      \
	void pmc_store ## size (void *addrs, uint ## size ## _t val){                                                   \
		DEBUG("pmc_store%u:addr = %p %lld\n", size, addrs, (uint64_t) val);  \
		createModelIfNotExist();                                \
		Thread *thrd = thread_current();                        \
		ModelAction * action = new ModelAction(NONATOMIC_WRITE, memory_order_relaxed, addrs, val, thrd, size); \
		model->switch_to_master(action);                                                                                        \
		*((volatile uint ## size ## _t *)addrs) = val;                  \
		*((volatile uint ## size ## _t *)lookupShadowEntry(addrs)) = val; \
		thread_id_t tid = thrd->get_id();               \
		raceCheckWrite ## size (tid, addrs);    \
	}

PMCHECKSTORE(8)
PMCHECKSTORE(16)
PMCHECKSTORE(32)
PMCHECKSTORE(64)


// PMC Non-Atomic Load

#define PMCHECKLOAD(size)                                               \
	uint ## size ## _t pmc_load ## size (void *addrs) {                   \
		DEBUG("pmc_load%u:addr = %p\n", size, addrs);                       \
		createModelIfNotExist();                                            \
		ModelAction *action = new ModelAction(NONATOMIC_READ, NULL, memory_order_relaxed, addrs, VALUE_NONE, size); \
		uint ## size ## _t val = (uint ## size ## _t)model->switch_to_master(action); \
		DEBUG("pmc_load: addr = %p val = %llu val2 = %llu\n", addrs, (uintptr_t) val, *((uintptr_t *)addrs)); \
		thread_id_t tid = thread_current()->get_id();                       \
		raceCheckRead ## size (tid, (void *)(((uintptr_t)addrs)));          \
		return val;                                                         \
	}

PMCHECKLOAD(8)
PMCHECKLOAD(16)
PMCHECKLOAD(32)
PMCHECKLOAD(64)

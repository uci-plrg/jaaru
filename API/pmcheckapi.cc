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
		createModelIfNotExist();                                                                                                                                                \
		Thread *thread = thread_current();                                                                                              \
		ModelAction * action = new ModelAction(NONATOMIC_WRITE, memory_order_relaxed, addrs, val, thread);        \
		model->switch_to_master(action);                                                                                        \
		*((volatile uint ## size ## _t *)addrs) = val;                  \
		*((volatile uint ## size ## _t *)lookupShadowEntry(addrs)) = val; \
		raceCheckWrite ## size (thread->get_id(), addrs);                                                               \
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
		ModelAction *action = new ModelAction(NONATOMIC_READ, NULL, memory_order_relaxed, addrs); \
		uint ## size ## _t val = (uint ## size ## _t)model->switch_to_master(action); \
		thread_id_t tid = thread_current()->get_id();                       \
		raceCheckRead ## size (tid, (void *)(((uintptr_t)addrs)));          \
		return val;                                                         \
	}

PMCHECKLOAD(8)
PMCHECKLOAD(16)
PMCHECKLOAD(32)
PMCHECKLOAD(64)

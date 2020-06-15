
#include <stdio.h>
#include "atomicapi.h"
#include "model.h"
#include "snapshot-interface.h"
#include "common.h"
#include "action.h"
#include "datarace.h"
#include "threads-model.h"

memory_order orders[7] = {
	memory_order_relaxed, memory_order_consume, memory_order_acquire,
	memory_order_release, memory_order_acq_rel, memory_order_seq_cst,
};


void createModelIfNotExist() {
	if (!model) {
		snapshot_system_init(10000, 1024, 1024, 40000);
		model = new ModelChecker();
		model->startChecker();
	}
}


// pmc volatile loads
#define VOLATILELOAD(size) \
	uint ## size ## _t pmc_volatile_load ## size(void * obj, const char * position) {                                               \
		DEBUG("pmc_volatile_load%u:addr = %p\n", size, obj); \
		createModelIfNotExist();                                                                                                \
		ModelAction *action = new ModelAction(ATOMIC_READ, position, memory_order_volatile_load, obj, size);                          \
		return (uint ## size ## _t)model->switch_to_master(action);                                                             \
	}

VOLATILELOAD(8)
VOLATILELOAD(16)
VOLATILELOAD(32)
VOLATILELOAD(64)

// pmc volatile stores
#define VOLATILESTORE(size) \
	void pmc_volatile_store ## size (void * obj, uint ## size ## _t val, const char * position) {                                   \
		DEBUG("pmc_volatile_store%u:addr = %p, value= %" PRIu ## size "\n", size, obj, val); \
		createModelIfNotExist();                                                                                                \
		ModelAction *action = new ModelAction(ATOMIC_WRITE, position, memory_order_volatile_store, obj, (uint64_t) val, size);        \
		model->switch_to_master(action);                                                                                        \
		*((volatile uint ## size ## _t *)obj) = val;                                                                            \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size / 8;i++) {                       \
			atomraceCheckWrite(tid, (void *)(((char *)obj)+i));          \
		}                                               \
	}

VOLATILESTORE(8)
VOLATILESTORE(16)
VOLATILESTORE(32)
VOLATILESTORE(64)

// pmc atomic inits
#define PMCATOMICINT(size)                                              \
	void pmc_atomic_init ## size (void * obj, uint ## size ## _t val, const char * position) { \
		DEBUG("pmc_atomic_init%u:addr = %p, value= %" PRIu ## size "\n", size, obj, val); \
		createModelIfNotExist();                                                      \
		ModelAction *action = new ModelAction(ATOMIC_INIT, position, memory_order_relaxed, obj, (uint64_t) val, size);                \
		model->switch_to_master(action);                                                                                        \
		*((volatile uint ## size ## _t *)obj) = val;                                                                            \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size / 8;i++) {                       \
			atomraceCheckWrite(tid, (void *)(((char *)obj)+i));          \
		}                                                                                                                       \
	}

PMCATOMICINT(8)
PMCATOMICINT(16)
PMCATOMICINT(32)
PMCATOMICINT(64)


// pmc atomic loads
#define PMCATOMICLOAD(size)                                             \
	uint ## size ## _t pmc_atomic_load ## size(void * obj, int atomic_index, const char * position) {                               \
		DEBUG("pmc_atomic_load%u:addr = %p\n", size, obj); \
		createModelIfNotExist();                                                                                                \
		ModelAction *action = new ModelAction(ATOMIC_READ, position, orders[atomic_index], obj, size);                                \
		uint ## size ## _t val = (uint ## size ## _t)model->switch_to_master(action);                                           \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size / 8;i++) {                         \
			atomraceCheckRead(tid, (void *)(((char *)obj)+i));    \
		}                                                       \
		return val;                                                                                                             \
	}

PMCATOMICLOAD(8)
PMCATOMICLOAD(16)
PMCATOMICLOAD(32)
PMCATOMICLOAD(64)


// pmc atomic stores
#define PMCATOMICSTORE(size)                                            \
	void pmc_atomic_store ## size(void * obj, uint ## size ## _t val, int atomic_index, const char * position) {                    \
		DEBUG("pmc_atomic_store%u:addr = %p, value= %" PRIu ## size "\n", size, obj, val); \
		createModelIfNotExist();                                                                                                \
		ModelAction *action =  new ModelAction(ATOMIC_WRITE, position, orders[atomic_index], obj, (uint64_t) val, size);              \
		model->switch_to_master(action);                                                                                        \
		*((volatile uint ## size ## _t *)obj) = val;                                                                            \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size / 8;i++) {                       \
			atomraceCheckWrite(tid, (void *)(((char *)obj)+i));          \
		}                                                       \
	}

PMCATOMICSTORE(8)
PMCATOMICSTORE(16)
PMCATOMICSTORE(32)
PMCATOMICSTORE(64)


#define _ATOMIC_RMW_(__op__, size, addr, val, atomic_index, position, rmw_type)                                                                   \
	({                                                                                                                              \
		DEBUG("pmc_atomic_RMW_%u:addr = %p, value= %" PRIu ## size "\n", size, addr, val); \
		createModelIfNotExist();																								\
		ModelAction* action = new ModelAction(ATOMIC_RMW, position, orders[atomic_index], addr, val, size, rmw_type);							\
		uint ## size ## _t old = model->switch_to_master(action);																\
		uint ## size ## _t newVal = old;																						\
		newVal __op__ val;																										\
		*((volatile uint ## size ## _t *)addr) = newVal;                                                                         \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size / 8;i++) {                       \
			atomraceCheckRead(tid,  (void *)(((char *)addr)+i));  \
			recordWrite(tid, (void *)(((char *)addr)+i));         \
		}                                                               \
		return old;                                            \
	})

// pmc atomic exchange
#define PMCATOMICEXCHANGE(size)                                         \
	uint ## size ## _t cds_atomic_exchange ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( =, size, addr, val, atomic_index, position, RMW_EXCHANGE);          \
	}

PMCATOMICEXCHANGE(8)
PMCATOMICEXCHANGE(16)
PMCATOMICEXCHANGE(32)
PMCATOMICEXCHANGE(64)


// pmc atomic fetch add
#define PMCATOMICADD(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_add ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( +=, size, addr, val, atomic_index, position, RMW_ADD);         \
	}

PMCATOMICADD(8)
PMCATOMICADD(16)
PMCATOMICADD(32)
PMCATOMICADD(64)

// pmc atomic fetch sub
#define PMCATOMICSUB(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_sub ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( -=, size, addr, val, atomic_index, position, RMW_SUB);         \
	}

PMCATOMICSUB(8)
PMCATOMICSUB(16)
PMCATOMICSUB(32)
PMCATOMICSUB(64)

// pmc atomic fetch and
#define PMCATOMICAND(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_and ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( &=, size, addr, val, atomic_index, position, RMW_AND);         \
	}

PMCATOMICAND(8)
PMCATOMICAND(16)
PMCATOMICAND(32)
PMCATOMICAND(64)

// pmc atomic fetch or
#define PMCATOMICOR(size)                                               \
	uint ## size ## _t pmc_atomic_fetch_or ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( |=, size, addr, val, atomic_index, position, RMW_OR);         \
	}

PMCATOMICOR(8)
PMCATOMICOR(16)
PMCATOMICOR(32)
PMCATOMICOR(64)

// pmc atomic fetch xor
#define PMCATOMICXOR(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_xor ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( ^=, size, addr, val, atomic_index, position, RMW_XOR);         \
	}

PMCATOMICXOR(8)
PMCATOMICXOR(16)
PMCATOMICXOR(32)
PMCATOMICXOR(64)

// pmc atomic compare and exchange
// In order to accomodate the LLVM PASS, the return values are not true or false.
// TODO: Support will be added later.

#define _ATOMIC_CMPSWP_WEAK_ _ATOMIC_CMPSWP_
#define _ATOMIC_CMPSWP_(size, addr, expected, desired, atomic_index, position)                            \
	({                                                                                              \
		DEBUG("pmc_atomic_COMPSWP%u:addr = %p, value= %" PRIu ## size "\n", size, addr, desired); \
		ASSERT(0);																					\
		return 0;																					\
	})

// atomic_compare_exchange version 1: the CmpOperand (corresponds to expected)
// extracted from LLVM IR is an integer type.
#define CDSATOMICCASV1(size)                                            \
	uint ## size ## _t pmc_atomic_compare_exchange ## size ## _v1(void* addr, uint ## size ## _t expected, uint ## size ## _t desired, int atomic_index_succ, int atomic_index_fail, const char *position) { \
		_ATOMIC_CMPSWP_(size, addr, expected, desired, atomic_index_succ, position); \
	}

CDSATOMICCASV1(8)
CDSATOMICCASV1(16)
CDSATOMICCASV1(32)
CDSATOMICCASV1(64)

// atomic_compare_exchange version 2
#define CDSATOMICCASV2(size)                                            \
	bool pmc_atomic_compare_exchange ## size ## _v2(void* addr, uint ## size ## _t* expected, uint ## size ## _t desired, int atomic_index_succ, int atomic_index_fail, const char *position) { \
		uint ## size ## _t ret = pmc_atomic_compare_exchange ## size ## _v1(addr, *expected, desired, atomic_index_succ, atomic_index_fail, position); \
		if (ret == *expected) {return true;} else {return false;}               \
	}

CDSATOMICCASV2(8)
CDSATOMICCASV2(16)
CDSATOMICCASV2(32)
CDSATOMICCASV2(64)


// PMC NVM operations

void pmc_clwb(void * addrs){
	DEBUG("pmc_clwb:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLWB, memory_order_seq_cst, addrs);
	model->switch_to_master(action);
}

void pmc_clflushopt(void * addrs){
	DEBUG("pmc_clflushopt:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLFLUSHOPT, memory_order_seq_cst, addrs);
	model->switch_to_master(action);
}

void pmc_clflush(void * addrs){
	DEBUG("pmc_clflush:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLFLUSH, memory_order_seq_cst, addrs);
	model->switch_to_master(action);
}

void pmc_mfence(){
	DEBUG("pmc_mfence\n");
	createModelIfNotExist();
	ModelAction *action = new ModelAction(CACHE_MFENCE) ;
	model->switch_to_master(action);
}

void pmc_lfence(){
	DEBUG("pmc_lfence\n");
	ASSERT(0);
}

void pmc_sfence(){
	DEBUG("pmc_sfence\n");
	createModelIfNotExist();
	ModelAction *action = new ModelAction(CACHE_SFENCE) ;
	model->switch_to_master(action);
}


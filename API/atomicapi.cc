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


// pmc volatile loads
#define VOLATILELOAD(size) \
	uint ## size ## _t pmc_volatile_load ## size(void * obj, const char * position) {                                               \
		DEBUG("pmc_volatile_load%u:addr = %p\n", size, obj); \
		createModelIfNotExist();                                                                                                \
		ModelAction *action = new ModelAction(ATOMIC_READ, position, memory_order_volatile_load, obj, VALUE_NONE, size>>3); \
		uint ## size ## _t val = (uint ## size ## _t)model->switch_thread(action);                                                             \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size >> 3;i++) {                        \
			atomraceCheckRead(tid, (void *)(((char *)obj)+i));    \
		}                                                       \
		return val; \
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
		ModelAction *action = new ModelAction(ATOMIC_WRITE, position, memory_order_volatile_store, obj, (uint64_t) val, size>>3); \
		model->switch_thread(action);                        \
		*((volatile uint ## size ## _t *)obj) = val;            \
		*((volatile uint ## size ## _t *)lookupShadowEntry(obj)) = val; \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0 ;i < size >> 3;i++) {                       \
			atomraceCheckWrite (tid, (void *)(((char *)obj)+i));                                                                                                                   \
		}                                                                                                                                                                                                                               \
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
		ModelAction *action = new ModelAction(ATOMIC_INIT, position, memory_order_relaxed, obj, (uint64_t) val, size>>3); \
		model->switch_thread(action);                                                                                        \
		*((uint ## size ## _t *)obj) = val;                     \
		*((uint ## size ## _t *)lookupShadowEntry(obj)) = val;  \
		thread_id_t tid = thread_current()->get_id();                           \
		for(int i=0 ;i < size >> 3;i++) {                       \
			atomraceCheckWrite (tid, (void *)(((char *)obj)+i));                                                                                                                   \
		}                                                                                                                                                                       \
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
		ModelAction *action = new ModelAction(ATOMIC_READ, position, orders[atomic_index], obj, VALUE_NONE, size>>3); \
		uint ## size ## _t val = (uint ## size ## _t)model->switch_thread(action);                                           \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size >> 3;i++) {                        \
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
		ModelAction *action =  new ModelAction(ATOMIC_WRITE, position, orders[atomic_index], obj, (uint64_t) val, size >> 3); \
		model->switch_thread(action);                                                                                        \
		*((volatile uint ## size ## _t *)obj) = val;                    \
		*((volatile uint ## size ## _t *)lookupShadowEntry(obj)) = val; \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0 ;i < size >> 3;i++) {                       \
			atomraceCheckWrite (tid, (void *)(((char *)obj)+i));                                                                                                                   \
		}                                                                                                                                               \
	}

PMCATOMICSTORE(8)
PMCATOMICSTORE(16)
PMCATOMICSTORE(32)
PMCATOMICSTORE(64)


uint64_t model_rmw_read_action(void *obj, int atomic_index, const char *position, int size) {
	createModelIfNotExist();
	return model->switch_thread(new ModelAction(ATOMIC_RMWR, position, orders[atomic_index], obj, VALUE_NONE, size));
}

ModelAction* model_rmw_action(void *obj, uint64_t val, int atomic_index, const char * position, int size) {
	createModelIfNotExist();
	ModelAction* action = new ModelAction(ATOMIC_RMW, position, orders[atomic_index], obj, val, size);
	model->switch_thread(action);
	return action;
}

#define _ATOMIC_RMW_(__op__, size, addr, val, atomic_index, position)                                                                   \
	({                                                                                                                              \
		DEBUG("pmc_atomic_RMW_%u:addr = %p, value= %" PRIu ## size "\n", size, addr, val); \
		uint ## size ## _t _old = model_rmw_read_action(addr, atomic_index, position, size>>3); \
		uint ## size ## _t _copy = _old;                                                                                        \
		uint ## size ## _t _val = val;                                                                                          \
		_copy __op__ _val;                                                                                                      \
		ModelAction *action = model_rmw_action(addr, (uint64_t) _copy, atomic_index, position, size>>3); \
		*((volatile uint ## size ## _t *)addr) = _copy;         \
		*((volatile uint ## size ## _t *)lookupShadowEntry(addr)) = _copy; \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size >> 3;i++) {                        \
			atomraceCheckRead(tid,  (void *)(((char *)addr)+i));  \
			recordWrite(tid, (void *)(((char *)addr)+i));         \
		}                                                               \
		return _old;                                            \
	})

// pmc atomic exchange
#define PMCATOMICEXCHANGE(size)                                         \
	uint ## size ## _t pmc_atomic_exchange ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( =, size, addr, val, atomic_index, position);          \
	}

PMCATOMICEXCHANGE(8)
PMCATOMICEXCHANGE(16)
PMCATOMICEXCHANGE(32)
PMCATOMICEXCHANGE(64)


// pmc atomic fetch add
#define PMCATOMICADD(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_add ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( +=, size, addr, val, atomic_index, position);         \
	}

PMCATOMICADD(8)
PMCATOMICADD(16)
PMCATOMICADD(32)
PMCATOMICADD(64)

// pmc atomic fetch sub
#define PMCATOMICSUB(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_sub ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( -=, size, addr, val, atomic_index, position);         \
	}

PMCATOMICSUB(8)
PMCATOMICSUB(16)
PMCATOMICSUB(32)
PMCATOMICSUB(64)

// pmc atomic fetch and
#define PMCATOMICAND(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_and ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( &=, size, addr, val, atomic_index, position);         \
	}

PMCATOMICAND(8)
PMCATOMICAND(16)
PMCATOMICAND(32)
PMCATOMICAND(64)

// pmc atomic fetch or
#define PMCATOMICOR(size)                                               \
	uint ## size ## _t pmc_atomic_fetch_or ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( |=, size, addr, val, atomic_index, position);         \
	}

PMCATOMICOR(8)
PMCATOMICOR(16)
PMCATOMICOR(32)
PMCATOMICOR(64)

// pmc atomic fetch xor
#define PMCATOMICXOR(size)                                              \
	uint ## size ## _t pmc_atomic_fetch_xor ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( ^=, size, addr, val, atomic_index, position);         \
	}

PMCATOMICXOR(8)
PMCATOMICXOR(16)
PMCATOMICXOR(32)
PMCATOMICXOR(64)

void model_rmw_cas_fail_action(void *obj, int atomic_index, const char *position, int size) {
	createModelIfNotExist();
	model->switch_thread(new ModelAction(ATOMIC_CAS_FAILED, position, orders[atomic_index], obj, size));
}

// pmc atomic compare and exchange
// In order to accomodate the LLVM PASS, the return values are not true or false.

#define _ATOMIC_CMPSWP_WEAK_ _ATOMIC_CMPSWP_
#define _ATOMIC_CMPSWP_(size, addr, expected, desired, atomic_index, position)                            \
	({                                                                                              \
		DEBUG("pmc_atomic_COMPSWP%u:addr = %p, value= %" PRIu ## size "\n", size, addr, desired); \
		uint ## size ## _t _desired = desired;                                                            \
		uint ## size ## _t _expected = expected;                                                          \
		uint ## size ## _t _old = model_rmw_read_action(addr, atomic_index, position, size>>3); \
		thread_id_t tid = thread_current()->get_id();           \
		for(int i=0;i < size >> 3;i++) {                        \
			atomraceCheckRead(tid,  (void *)(((char *)addr)+i));  \
		}                                                               \
		if (_old == _expected) {                                                                    \
			ModelAction *action = model_rmw_action(addr, (uint64_t) _desired, atomic_index, position, size>>3); \
			*((volatile uint ## size ## _t *)addr) = desired; \
			*((volatile uint ## size ## _t *)lookupShadowEntry(addr)) = desired;    \
			for(int i=0;i < size >> 3;i++) {                \
				recordWrite(tid, (void *)(((char *)addr)+i));         \
			} \
			return _expected; }                                     \
		else {                                                                                        \
			model_rmw_cas_fail_action(addr, atomic_index, position, size>>3); _expected = _old; return _old; } \
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


// pmc atomic thread fence

void pmc_atomic_thread_fence(int atomic_index, const char * position) {
	DEBUG("pmc_atomic_thread_fence\n"); \
}
// PMC NVM operations

void pmc_clwb(void * addrs){
	DEBUG("pmc_clwb:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLFLUSHOPT, memory_order_seq_cst, addrs);
	model->switch_thread(action);
}

void pmc_clflushopt(void * addrs){
	DEBUG("pmc_clflushopt:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLFLUSHOPT, memory_order_seq_cst, addrs);
	model->switch_thread(action);
}

void pmc_clflush(void * addrs){
	DEBUG("pmc_clflush:addr = %p\n",addrs);
	createModelIfNotExist();
	ModelAction *action = new ModelAction(ACTION_CLFLUSH, memory_order_seq_cst, addrs);
	model->switch_thread(action);
}

void pmc_mfence(){
	DEBUG("pmc_mfence\n");
	createModelIfNotExist();
	ModelAction *action = new ModelAction(CACHE_MFENCE) ;
	model->switch_thread(action);
}

void pmc_lfence(){
	DEBUG("pmc_lfence\n");
	ASSERT(0);
}

void pmc_sfence(){
	DEBUG("pmc_sfence\n");
	createModelIfNotExist();
	ModelAction *action = new ModelAction(CACHE_SFENCE) ;
	model->switch_thread(action);
}


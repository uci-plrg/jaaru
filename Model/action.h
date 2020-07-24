/** @file action.h
 *  @brief Models actions taken by threads.
 */

#ifndef __ACTION_H__
#define __ACTION_H__

#include <cstddef>
#include <inttypes.h>
#include "classlist.h"
#include "mypthread.h"
#include "mymemory.h"
#include "memoryorder.h"
#include "mutex.h"
using std::memory_order;
using std::memory_order_relaxed;
using std::memory_order_consume;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;
/**
 * @brief A recognizable don't-care value for use in the ModelAction::value
 * field
 *
 * Note that this value can be legitimately used by a program, and hence by
 * iteself does not indicate no value.
 */
#define VALUE_NONE 0xdeadbeef

/** @brief Represents an action type, identifying one of several types of
 * ModelAction */

typedef enum action_type {
	THREAD_CREATE,	// < A thread creation action
	THREAD_START,	// < First action in each thread
	THREAD_YIELD,	// < A thread yield action
	THREAD_JOIN,	// < A thread join action
	THREAD_FINISH,	// < A thread completion action
	THREADONLY_FINISH,	// < A thread completion action
	THREAD_SLEEP,	// < A sleep operation

	PTHREAD_CREATE,	// < A pthread creation action
	PTHREAD_JOIN,	// < A pthread join action

	NONATOMIC_WRITE,	// Represents a non-atomic store
	NONATOMIC_READ,	// Represents a non-atomic load

	ATOMIC_INIT,	// < Initialization of an atomic object (e.g., atomic_init())
	ATOMIC_WRITE,	// < An atomic write action
	ATOMIC_RMWR,	// Read part of an atomic RMW action
	ATOMIC_CAS_FAILED,
	ATOMIC_RMW,	// < The write part of an atomic RMW action
	ATOMIC_READ,	// < An atomic read action

	ACTION_CLFLUSH,
	ACTION_CLFLUSHOPT,

	CACHE_MFENCE,
	CACHE_SFENCE,

	ATOMIC_LOCK,	// < A lock action
	ATOMIC_TRYLOCK,	// < A trylock action
	ATOMIC_UNLOCK,	// < An unlock action

	ATOMIC_NOTIFY_ONE,	// < A notify_one action
	ATOMIC_NOTIFY_ALL,	// < A notify all action
	ATOMIC_WAIT,	// < A wait action
	ATOMIC_TIMEDWAIT,	// < A timed wait action
	ATOMIC_ANNOTATION,	// < An annotation action to pass information to a trace analysis
	ATOMIC_NOP	// < Placeholder
} action_type_t;


/**
 * @brief Represents a single atomic action
 *
 * A ModelAction is always allocated as non-snapshotting, because it is used in
 * multiple executions during backtracking. Except for non-atomic write
 * ModelActions, each action is assigned a unique sequence
 * number.
 */
class ModelAction {
public:
	ModelAction(action_type_t type, memory_order order, void *loc, uint64_t value = VALUE_NONE, Thread *thread = NULL, uint size = 0);
	ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value, int size);
	ModelAction(action_type_t type);
	ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value = VALUE_NONE, Thread *thread = NULL);
	~ModelAction();
	void print() const;

	thread_id_t get_tid() const { return tid; }
	action_type get_type() const { return type; }
	void set_type(action_type _type) { type = _type; }
	memory_order get_mo() const { return order; }
	void set_mo(memory_order order) { this->order = order; }
	void * get_location() const { return location; }
	const char * get_position() const { return position; }
	modelclock_t get_seq_number() const ;
	uint64_t get_value() const;
	uint64_t get_reads_from_value() const;
	uint64_t get_write_value() const;
	uint64_t get_return_value() const;
	ModelAction * get_reads_from() const { return reads_from; }
	modelclock_t get_last_write() const { return lastcommittedWrite; };
	uint64_t get_time() const {return time;}
	pmc::mutex * get_mutex() const;

	void set_read_from(ModelAction *act);

	void copy_from_new(ModelAction *newaction);
	void set_seq_number(modelclock_t num);
	void reset_seq_number();
	void set_try_lock(bool obtainedlock);
	bool is_thread_start() const;
	bool is_thread_join() const;
	bool is_mutex_op() const;
	bool is_lock() const;
	bool is_sleep() const;
	bool is_trylock() const;
	bool is_unlock() const;
	bool is_wait() const;
	bool is_create() const;
	bool is_notify() const;
	bool is_notify_one() const;
	bool is_success_lock() const;
	bool is_failed_trylock() const;
	bool is_read() const;
	bool is_write() const;
	bool is_nonatomic_write() const;
	bool is_cache_op() const;
	bool is_clflush() const;
	bool is_mfence() const;
	bool is_sfence() const;
	bool is_yield() const;
	bool is_locked_operation() const;
	bool is_rmw() const;
	bool is_rmw_cas_fail() const;
	bool is_rmw_read() const;
	bool is_initialization() const;
	bool is_annotation() const;
	bool is_seqcst() const;
	bool is_executed() const;
	bool same_var(const ModelAction *act) const;
	bool same_thread(const ModelAction *act) const;
	bool is_conflicting_lock(const ModelAction *act) const;
	bool could_synchronize_with(const ModelAction *act) const;
	int getOpSize() const;
	Thread * get_thread_operand() const;
	void create_cv(const ModelAction *parent = NULL);
	ClockVector * get_cv() const { return cv; }
	bool synchronize_with(const ModelAction *act);

	bool has_synchronized_with(const ModelAction *act) const;
	bool happens_before(const ModelAction *act) const;

	inline bool operator <(const ModelAction& act) const {
		return get_seq_number() < act.get_seq_number();
	}
	inline bool operator >(const ModelAction& act) const {
		return get_seq_number() > act.get_seq_number();
	}
	void process_rmw(ModelAction * act);
	void copy_typeandorder(ModelAction * act);
	unsigned int hash() const;
	bool equals(const ModelAction *x) const { return this == x; }
	void set_value(uint64_t val) { value = val; }

	/* to accomodate pthread create and join */
	Thread * thread_operand;
	void set_thread_operand(Thread *th) { thread_operand = th; }
	void setActionRef(sllnode<ModelAction *> *ref) { action_ref = ref; }
	sllnode<ModelAction *> * getActionRef() { return action_ref; }
	void setLastWrites(modelclock_t mlastcommittedWrite, ModelAction *write) {
		lastcommittedWrite = mlastcommittedWrite;
		reads_from = write;
	}
	modelclock_t get_last_clflush() { return last_clflush; }

	MEMALLOC
private:
	const char * get_type_str() const;
	const char * get_mo_str() const;

	/** @brief A pointer to the memory location for this action. */
	void *location;

	/** @brief A pointer to the source line for this atomic action. */
	const char * position;
	/**
	 * @brief The store that this action reads from
	 *
	 * Only valid for reads
	 */
	ModelAction *reads_from;
	union {
		uint64_t time;	//used for sleep
		modelclock_t lastcommittedWrite;
	};
/**
 * @brief The clock vector for this operation
 *
 * Technically, this is only needed for potentially synchronizing
 * (e.g., non-relaxed) operations, but it is very handy to have these
 * vectors for all operations.
 */
	ClockVector *cv;
	sllnode<ModelAction *> * action_ref;
	/** @brief The value written (for write or RMW; undefined for read) */
	union {
		uint64_t value;
		modelclock_t last_clflush;
	};
	/** @brief Type of action (read, write, RMW, fence, thread create, etc.) */
	action_type type;

	/** @brief The memory order for this operation. */
	memory_order order;

	/** @brief The thread id that performed this action. */
	thread_id_t tid;

	/**
	 * @brief The sequence number of this action
	 *
	 * Except for non atomic write actions, this number should be unique and
	 * should represent the action's position in the execution order.
	 */
	modelclock_t seq_number;
	/**
	 * @brief Size of this action
	 * If the action is write or read, we keep the size of the operation here.
	 * (e.g. 8, 16, 32, or 64)
	 */
	uint size;
};

#endif	/* __ACTION_H__ */

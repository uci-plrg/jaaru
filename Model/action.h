/** @file action.h
 *  @brief Models actions taken by threads.
 */

#ifndef __ACTION_H__
#define __ACTION_H__

#include <cstddef>
#include <inttypes.h>

#include "mypthread.h"
#include "mymemory.h"
#include "memoryorder.h"

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
/**
 * @brief The "location" at which a fence occurs
 *
 * We need a non-zero memory location to associate with fences, since our hash
 * tables don't handle NULL-pointer keys. HACK: Hopefully this doesn't collide
 * with any legitimate memory locations.
 */
#define FENCE_LOCATION ((void *)0x7)

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
	NONATOMIC_READ,		// Represents a non-atomic load

	ATOMIC_INIT,	// < Initialization of an atomic object (e.g., atomic_init())
	ATOMIC_WRITE,	// < An atomic write action
	ATOMIC_RMW,	// < The write part of an atomic RMW action
	ATOMIC_READ,	// < An atomic read action
	ATOMIC_RMWR,	// < The read part of an atomic RMW action
	ATOMIC_RMWRCAS,	// < The read part of an atomic RMW action
	ATOMIC_RMWC,	// < Convert an atomic RMW action into a READ

	ACTION_CLWB,
	ACTION_CLFLUSH,
	ACTION_CLFLUSHOPT,

	CACHE_MFENCE,
	CACHE_SFENCE,

	ATOMIC_FENCE,	// < A fence action
	ATOMIC_LOCK,	// < A lock action
	ATOMIC_TRYLOCK,	// < A trylock action
	ATOMIC_UNLOCK,	// < An unlock action

	ATOMIC_NOTIFY_ONE,	// < A notify_one action
	ATOMIC_NOTIFY_ALL,	// < A notify all action
	ATOMIC_WAIT,	// < A wait action
	ATOMIC_TIMEDWAIT,	// < A timed wait action
	ATOMIC_ANNOTATION,	// < An annotation action to pass information to a trace analysis
	READY_FREE,	// < Write is ready to be freed
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
	ModelAction(action_type_t type, memory_order order, void *loc, uint64_t value = VALUE_NONE, Thread *thread = NULL);
	ModelAction(action_type_t type, memory_order order, void *loc, uint64_t value, int size);
	ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value, int size);
	ModelAction(action_type_t type, memory_order order, uint64_t value, uint64_t time);
	ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value = VALUE_NONE, Thread *thread = NULL);
	~ModelAction();
	void print() const;

	thread_id_t get_tid() const { return tid; }
	action_type get_type() const { return type; }
	void set_type(action_type _type) { type = _type; }
	void set_free() { type = READY_FREE; }
	void * get_location() const { return location; }
	const char * get_position() const { return position; }
	modelclock_t get_seq_number() const { return seq_number; }
	uint64_t get_value() const { return value; }
	uint64_t get_reads_from_value() const;
	uint64_t get_write_value() const;
	uint64_t get_return_value() const;
	ModelAction * get_reads_from() const { return reads_from; }
	uint64_t get_time() const {return time;}
	void set_read_from(ModelAction *act);
	void set_seq_number(modelclock_t num);
	void reset_seq_number();
	bool is_read() const;
	bool is_write() const;
	bool is_rmwr() const;
	bool is_rmwrcas() const;
	bool is_rmwc() const;
	bool is_rmw() const;
	bool is_fence() const;
	bool same_var(const ModelAction *act) const;
	bool same_thread(const ModelAction *act) const;
	int getSize() const;
	Thread * get_thread_operand() const;
	inline bool operator <(const ModelAction& act) const {
		return get_seq_number() < act.get_seq_number();
	}
	inline bool operator >(const ModelAction& act) const {
		return get_seq_number() > act.get_seq_number();
	}

	unsigned int hash() const;
	bool equals(const ModelAction *x) const { return this == x; }
	void set_value(uint64_t val) { value = val; }

	SNAPSHOTALLOC
private:
	const char * get_type_str() const;
	const char * get_mo_str() const;

	/** @brief A pointer to the memory location for this action. */
	void *location;

	/** @brief A pointer to the source line for this atomic action. */
	const char * position;

	union {
		/**
		 * @brief The store that this action reads from
		 *
		 * Only valid for reads
		 */
		ModelAction *reads_from;
		int size;
		uint64_t time;  //used for sleep
	};

	/** @brief The value written (for write or RMW; undefined for read) */
	uint64_t value;

	/** @brief Type of action (read, write, RMW, fence, thread create, etc.) */
	action_type type;

	/** @brief The original memory order parameter for this operation. */
	memory_order original_order;

	/** @brief The thread id that performed this action. */
	thread_id_t tid;

	/**
	 * @brief The sequence number of this action
	 *
	 * Except for non atomic write actions, this number should be unique and
	 * should represent the action's position in the execution order.
	 */
	modelclock_t seq_number;
};

#endif	/* __ACTION_H__ */

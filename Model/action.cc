#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>

#include "model.h"
#include "action.h"
#include "clockvector.h"
#include "common.h"
#include "threads-model.h"

#define ACTION_INITIAL_CLOCK 0

/** @brief A special value to represent a successful trylock */
#define VALUE_TRYSUCCESS 1

/** @brief A special value to represent a failed trylock */
#define VALUE_TRYFAILED 0

/**
 * @brief Construct a new ModelAction
 *
 * @param type The type of action
 * @param order The memory order of this action. A "don't care" for non-ATOMIC
 * actions (e.g., THREAD_* or MODEL_* actions).
 * @param loc The location that this action acts upon
 * @param value (optional) A value associated with the action (e.g., the value
 * read or written). Defaults to a given macro constant, for debugging purposes.
 * @param thread (optional) The Thread in which this action occurred. If NULL
 * (default), then a Thread is assigned according to the scheduler.
 */


ModelAction::ModelAction(action_type_t type, memory_order order, void *loc, uint64_t value, Thread *thread, uint _size) :
	location(loc),
	position(NULL),
	lastwrite(NULL),
	cv(NULL),
	action_ref(NULL),
	value(value),
	read_value(VALUE_NONE),
	type(type),
	order(order),
	seq_number(ACTION_INITIAL_CLOCK),
	size(_size)
{
	/* References to NULL atomic variables can end up here */
	if (loc == NULL && type != ATOMIC_NOP)
		model_print("Trying to access NULL pointer!\n");
	ASSERT(loc || type == ATOMIC_NOP);

	Thread *t = thread ? thread : thread_current();
	this->tid = t!= NULL ? t->get_id() : -1;
}


/**
 * @brief Construct a new ModelAction for cache fence
 *
 * @param type The type of action: CACHE_MFENCE
 */

ModelAction::ModelAction(action_type_t type) :
	location(NULL),
	position(NULL),
	time(0),
	cv(NULL),
	action_ref(NULL),
	value(0),
	read_value(VALUE_NONE),
	type(type),
	order(memory_order_seq_cst),
	seq_number(ACTION_INITIAL_CLOCK),
	size(0)
{
	Thread *t = thread_current();
	this->tid = t!= NULL ? t->get_id() : -1;
}


/**
 * @brief Construct a new ModelAction with source line number (requires llvm support)
 *
 * @param type The type of action
 * @param order The memory order of this action. A "don't care" for non-ATOMIC
 * actions (e.g., THREAD_* or MODEL_* actions).
 * @param loc The location that this action acts upon
 * @param value (optional) A value associated with the action (e.g., the value
 * read or written). Defaults to a given macro constant, for debugging purposes.
 * @param size (optional) The Thread in which this action occurred. If NULL
 * (default), then a Thread is assigned according to the scheduler.
 */
ModelAction::ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value, uint _size) :
	location(loc),
	position(position),
	lastwrite(NULL),
	cv(NULL),
	action_ref(NULL),
	value(value),
	read_value(VALUE_NONE),
	type(type),
	order(order),
	seq_number(ACTION_INITIAL_CLOCK),
	size(_size)
{
	/* References to NULL atomic variables can end up here */
	if (loc == NULL)
		model_print("Trying to access NULL pointer!\n");
	ASSERT(loc);
	Thread *t = thread_current();
	this->tid = t->get_id();
}


/**
 * @brief Construct a new ModelAction with source line number (requires llvm support)
 *
 * @param type The type of action
 * @param position The source line number of this atomic operation
 * @param order The memory order of this action. A "don't care" for non-ATOMIC
 * actions (e.g., THREAD_* or MODEL_* actions).
 * @param loc The location that this action acts upon
 * @param value (optional) A value associated with the action (e.g., the value
 * read or written). Defaults to a given macro constant, for debugging purposes.
 * @param thread (optional) The Thread in which this action occurred. If NULL
 * (default), then a Thread is assigned according to the scheduler.
 */
ModelAction::ModelAction(action_type_t type, const char * position, memory_order order, void *loc, uint64_t value, Thread *thread) :
	location(loc),
	position(position),
	lastwrite(NULL),
	cv(NULL),
	action_ref(NULL),
	value(value),
	read_value(VALUE_NONE),
	type(type),
	order(order),
	seq_number(ACTION_INITIAL_CLOCK),
	size(0)
{
	/* References to NULL atomic variables can end up here */
	if (loc == NULL)
		model_print("Trying to access NULL pointer!\n");
	ASSERT(loc);

	Thread *t = thread ? thread : thread_current();
	this->tid = t->get_id();
}

/**
 * @brief Construct a new ModelAction for sleep actions
 *
 * @param type The type of action: THREAD_SLEEP
 * @param order The memory order of this action. A "don't care" for non-ATOMIC
 * actions (e.g., THREAD_* or MODEL_* actions).
 * @param loc The location that this action acts upon
 * @param value The time duration a thread is scheduled to sleep.
 * @param _time The this sleep action is constructed
 */
ModelAction::ModelAction(action_type_t type, memory_order order, uint64_t value, uint64_t _time) :
	location(NULL),
	position(NULL),
	time(_time),
	cv(NULL),
	action_ref(NULL),
	value(value),
	type(type),
	order(order),
	seq_number(ACTION_INITIAL_CLOCK)
{
	Thread *t = thread_current();
	this->tid = t!= NULL ? t->get_id() : -1;
}

/** @brief ModelAction destructor */
ModelAction::~ModelAction() {
	if (cv)
		delete cv;
}

uint ModelAction::getOpSize() const {
	return size;
}

void ModelAction::copy_from_new(ModelAction *newaction)
{
	seq_number = newaction->seq_number;
}

void ModelAction::set_seq_number(modelclock_t num) {
	ASSERT(seq_number == ACTION_INITIAL_CLOCK);
	seq_number = num;
	if (cv != NULL)
	  cv->setClock(tid, num);
}

void ModelAction::reset_seq_number()
{
	seq_number = 0;
}

bool ModelAction::is_thread_start() const
{
	return type == THREAD_START;
}

bool ModelAction::is_thread_join() const
{
	return type == THREAD_JOIN || type == PTHREAD_JOIN;
}

bool ModelAction::is_mutex_op() const
{
	return type == ATOMIC_LOCK || type == ATOMIC_TRYLOCK || type == ATOMIC_UNLOCK || type == ATOMIC_WAIT || type == ATOMIC_TIMEDWAIT || type == ATOMIC_NOTIFY_ONE || type == ATOMIC_NOTIFY_ALL;
}

bool ModelAction::is_lock() const
{
	return type == ATOMIC_LOCK;
}

bool ModelAction::is_sleep() const
{
	return type == THREAD_SLEEP;
}

bool ModelAction::is_wait() const {
	return type == ATOMIC_WAIT || type == ATOMIC_TIMEDWAIT;
}

bool ModelAction::is_exit() const {
	return type == ACTION_EXIT;
}

bool ModelAction::is_notify() const {
	return type == ATOMIC_NOTIFY_ONE || type == ATOMIC_NOTIFY_ALL;
}

bool ModelAction::is_notify_one() const {
	return type == ATOMIC_NOTIFY_ONE;
}

bool ModelAction::is_unlock() const
{
	return type == ATOMIC_UNLOCK;
}

bool ModelAction::is_trylock() const
{
	return type == ATOMIC_TRYLOCK;
}

bool ModelAction::is_success_lock() const
{
	return type == ATOMIC_LOCK || (type == ATOMIC_TRYLOCK && value == VALUE_TRYSUCCESS);
}

bool ModelAction::is_failed_trylock() const
{
	return (type == ATOMIC_TRYLOCK && value == VALUE_TRYFAILED);
}

bool ModelAction::is_read() const
{
	return type == ATOMIC_READ || type == NONATOMIC_READ || type == ATOMIC_RMWR || type == ATOMIC_RMW;
}

bool ModelAction::is_write() const
{
	return type == ATOMIC_WRITE || type == ATOMIC_RMW || type == ATOMIC_INIT || type == NONATOMIC_WRITE;
}

bool ModelAction::is_atomic_write() const
{
	return type == ATOMIC_WRITE || type == ATOMIC_RMW || type == ATOMIC_INIT;
}

bool ModelAction::is_nonatomic_write() const
{
	return type == NONATOMIC_WRITE;
}

bool ModelAction::is_cache_op() const
{
	return type == ACTION_CLFLUSH || type == ACTION_CLFLUSHOPT;
}

bool ModelAction::is_fence() const
{
	if(is_write() && is_seqcst()) {
		return true;
	}
	switch(type)
	{
	case ATOMIC_RMW:
	case CACHE_MFENCE:
	case ATOMIC_CAS_FAILED:
	case PTHREAD_CREATE:
	case PTHREAD_JOIN:
	case THREAD_CREATE:
	case THREAD_JOIN:
	case ATOMIC_LOCK:
	case ATOMIC_TRYLOCK:
	case ATOMIC_UNLOCK:
	case ATOMIC_NOTIFY_ONE:
	case ATOMIC_NOTIFY_ALL:
	case ATOMIC_WAIT:
	case ATOMIC_TIMEDWAIT:
		return true;
	default:
		return false;
	}
}

bool ModelAction::is_mfence() const
{
	return type == CACHE_MFENCE;
}

bool ModelAction::is_sfence() const
{
	return type == CACHE_SFENCE;
}

bool ModelAction::is_clflush() const
{
	return type == ACTION_CLFLUSH;
}

bool ModelAction::is_locked_operation() const
{
	switch(type)
	{
	case PTHREAD_CREATE:
	case PTHREAD_JOIN:
	case THREAD_CREATE:
	case THREAD_JOIN:
	case ATOMIC_LOCK:
	case ATOMIC_TRYLOCK:
	case ATOMIC_UNLOCK:
	case ATOMIC_NOTIFY_ONE:
	case ATOMIC_NOTIFY_ALL:
	case ATOMIC_WAIT:
	case ATOMIC_TIMEDWAIT:
		return true;
	default:
		return false;
	}
	return false;
}


bool ModelAction::is_create() const
{
	return type == THREAD_CREATE || type == PTHREAD_CREATE;
}

bool ModelAction::is_yield() const
{
	return type == THREAD_YIELD;
}

bool ModelAction::is_rmw_read() const
{
	return type == ATOMIC_RMWR;
}

bool ModelAction::is_rmw_cas_fail() const
{
	return type == ATOMIC_CAS_FAILED;
}

bool ModelAction::is_rmw() const
{
	return type == ATOMIC_RMW;
}

bool ModelAction::is_initialization() const
{
	return type == ATOMIC_INIT;
}

bool ModelAction::is_annotation() const
{
	return type == ATOMIC_ANNOTATION;
}

bool ModelAction::is_seqcst() const
{
	return order == std::memory_order_seq_cst;
}

bool ModelAction::is_executed() const
{
	return seq_number != ACTION_INITIAL_CLOCK;
}

bool ModelAction::same_var(const ModelAction *act) const
{
	if (act->is_wait() || is_wait()) {
		if (act->is_wait() && is_wait()) {
			if (((void *)value) == ((void *)act->value))
				return true;
		} else if (is_wait()) {
			if (((void *)value) == act->location)
				return true;
		} else if (act->is_wait()) {
			if (location == ((void *)act->value))
				return true;
		}
	}

	return location == act->location;
}

bool ModelAction::same_thread(const ModelAction *act) const
{
	return tid == act->tid;
}

void ModelAction::copy_typeandorder(ModelAction * act)
{
	this->type = act->type;
	this->order = act->order;
}

/**
 * Get the Thread which is the operand of this action. This is only valid for
 * THREAD_* operations (currently only for THREAD_CREATE and THREAD_JOIN). Note
 * that this provides a central place for determining the conventions of Thread
 * storage in ModelAction, where we generally aren't very type-safe (e.g., we
 * store object references in a (void *) address.
 *
 * For THREAD_CREATE, this yields the Thread which is created.
 * For THREAD_JOIN, this yields the Thread we are joining with.
 *
 * @return The Thread which this action acts on, if exists; otherwise NULL
 */
Thread * ModelAction::get_thread_operand() const
{
	if (type == THREAD_CREATE) {
		/* thread_operand is set in execution.cc */
		return thread_operand;
	} else if (type == PTHREAD_CREATE) {
		return thread_operand;
	} else if (type == THREAD_JOIN) {
		/* THREAD_JOIN uses (Thread *) for location */
		return (Thread *)get_location();
	} else if (type == PTHREAD_JOIN) {
		// return NULL;
		// thread_operand is stored in execution::pthread_map;
		return (Thread *)get_location();
	} else
		return NULL;
}

/**
 * @brief Convert the read portion of an RMW
 *
 * Changes an existing read part of an RMW action into either:
 *  -# a full RMW action in case of the completed write or
 *  -# a READ action in case a failed action.
 *
 * @todo  If the memory_order changes, we may potentially need to update our
 * clock vector.
 *
 * @param act The second half of the RMW (either RMWC or RMW)
 */
void ModelAction::process_rmw(ModelAction *act)
{
	this->order = act->order;
	if (act->is_rmw_cas_fail())
		this->type = ATOMIC_READ;
	else if (act->is_rmw()) {
		this->type = ATOMIC_RMW;
		this->value = act->value;
	}
}

/**
 * @brief Check if this action should be backtracked with another, due to
 * potential synchronization
 *
 * The is_synchronizing method should only explore interleavings if:
 *  -# the operations are seq_cst and don't commute or
 *  -# the reordering may establish or break a synchronization relation.
 *
 * Other memory operations will be dealt with by using the reads_from relation.
 *
 * @param act The action to consider exploring a reordering
 * @return True, if we have to explore a reordering; otherwise false
 */
bool ModelAction::could_synchronize_with(const ModelAction *act) const
{
	// Same thread can't be reordered
	if (same_thread(act))
		return false;

	// Explore interleavings of seqcst writes/fences to guarantee total
	// order of seq_cst operations that don't commute
	if (is_write() || act->is_write() )
		return true;


	// lock just released...we can grab lock
	if ((is_lock() || is_trylock()) && (act->is_unlock() || act->is_wait()))
		return true;

	// lock just acquired...we can fail to grab lock
	if (is_trylock() && act->is_success_lock())
		return true;

	// other thread stalling on lock...we can release lock
	if (is_unlock() && (act->is_trylock() || act->is_lock()))
		return true;

	if (is_trylock() && (act->is_unlock() || act->is_wait()))
		return true;

	if (is_notify() && act->is_wait())
		return true;

	if (is_wait() && act->is_notify())
		return true;

	// Otherwise handle by reads_from relation
	return false;
}

bool ModelAction::is_conflicting_lock(const ModelAction *act) const
{
	// Must be different threads to reorder
	if (same_thread(act))
		return false;

	// Try to reorder a lock past a successful lock
	if (act->is_success_lock())
		return true;

	// Try to push a successful trylock past an unlock
	if (act->is_unlock() && is_trylock() && value == VALUE_TRYSUCCESS)
		return true;

	// Try to push a successful trylock past a wait
	if (act->is_wait() && is_trylock() && value == VALUE_TRYSUCCESS)
		return true;

	return false;
}

/**
 * Create a new clock vector for this action.
 *
 * @param parent A ModelAction from which to inherit a ClockVector
 */
void ModelAction::merge_cv(const ModelAction *parent)
{
	if (cv == NULL) {
		if (parent)
			cv = new ClockVector(parent->cv, this);
		else
			cv = new ClockVector(NULL, this);
	} else if (parent != NULL) {
		cv->merge(parent->cv);
	}
}

void ModelAction::merge_cv(ClockVector *mcv) {
	if (cv != NULL)
		cv->merge(mcv);
	else
		cv = new ClockVector(mcv, NULL);
}

void ModelAction::set_try_lock(bool obtainedlock)
{
	value = obtainedlock ? VALUE_TRYSUCCESS : VALUE_TRYFAILED;
}
/**
 * returning the action's seq_number. Making we never use an action
 * that the clock has not initialized yet.
 */
modelclock_t ModelAction::get_seq_number() const
{
	return seq_number;
}

/**
 * @brief Get the value written by this store
 *
 * We differentiate this function from ModelAction::get_reads_from_value and
 * ModelAction::get_value for the purpose of RMW's, which may have both a
 * 'read' and a 'write' value.
 *
 * Note: 'this' must be a store.
 *
 * @return The value written by this store
 */
uint64_t ModelAction::get_write_value() const
{
	ASSERT(is_write());
	return value;
}

uint64_t ModelAction::get_value() const {
	return value;
}

/**
 * Synchronize the current thread with the thread corresponding to the
 * ModelAction parameter.
 * @param act The ModelAction to synchronize with
 * @return True if this is a valid synchronization; false otherwise
 */
bool ModelAction::synchronize_with(const ModelAction *act)
{
	if (*this < *act)
		return false;
	cv->merge(act->cv);
	return true;
}

bool ModelAction::has_synchronized_with(const ModelAction *act) const
{
	return cv->synchronized_since(act);
}

/**
 * Check whether 'this' happens before act, according to the memory-model's
 * happens before relation. This is checked via the ClockVector constructs.
 * @return true if this action's thread has synchronized with act's thread
 * since the execution of act, false otherwise.
 */
bool ModelAction::happens_before(const ModelAction *act) const
{
	return act->cv->synchronized_since(this);
}

const char * ModelAction::get_type_str() const
{
	switch (this->type) {
	case THREAD_CREATE: return "thread create";
	case THREAD_START: return "thread start";
	case THREAD_YIELD: return "thread yield";
	case THREAD_JOIN: return "thread join";
	case THREAD_FINISH: return "thread finish";
	case THREAD_SLEEP: return "thread sleep";
	case THREADONLY_FINISH: return "pthread_exit finish";
	case ACTION_EXIT: return "action_exit";
	case PTHREAD_CREATE: return "pthread create";
	case PTHREAD_JOIN: return "pthread join";

	case NONATOMIC_WRITE: return "nonatomic write";
	case NONATOMIC_READ: return "nonatomic read";
	case ATOMIC_READ: return "atomic read";
	case ATOMIC_WRITE: return "atomic write";
	case ATOMIC_RMW: return "atomic rmw";
	case ATOMIC_RMWR: return "atomic rmwr";
	case ATOMIC_INIT: return "init atomic";
	case ATOMIC_LOCK: return "lock";
	case ATOMIC_UNLOCK: return "unlock";
	case ATOMIC_TRYLOCK: return "trylock";
	case ATOMIC_WAIT: return "wait";
	case ATOMIC_TIMEDWAIT: return "timed wait";
	case ATOMIC_NOTIFY_ONE: return "notify one";
	case ATOMIC_NOTIFY_ALL: return "notify all";
	case ATOMIC_ANNOTATION: return "annotation";
	case ACTION_CLFLUSH: return "clflush";
	case ACTION_CLFLUSHOPT: return "clflushopt";
	case CACHE_MFENCE: return "mfence";
	case CACHE_SFENCE: return "sfence";
	default: return "unknown type";
	};
}

const char * ModelAction::get_mo_str() const
{
	switch (this->order) {
	case std::memory_order_relaxed: return "relaxed";
	case std::memory_order_acquire: return "acquire";
	case std::memory_order_release: return "release";
	case std::memory_order_acq_rel: return "acq_rel";
	case std::memory_order_seq_cst: return "seq_cst";
	default: return "unknown";
	}
}

/** @brief Print nicely-formatted info about this ModelAction */
void ModelAction::print() const
{
	const char *type_str = get_type_str(), *mo_str = get_mo_str();

	model_print("%-4d %-2d   %-14s  %7s  %14p   %-#18" PRIx64 " %-2d",
							seq_number, id_to_int(tid), type_str, mo_str, location, get_value(), getOpSize());
	if (is_read()) {
		model_print(" %" PRIx64 " ", get_read_value());
	}
	if (cv) {
		if (is_read())
			model_print(" ");
		else
			model_print("      ");
		cv->print();
	} else
		model_print("\n");
	if(position) {
		model_print("%s\n", position);
	}
}

/** @brief Print nicely-formatted info about this ModelAction */
void ModelAction::printWithLocation() const
{
	const char *type_str = get_type_str(), *mo_str = get_mo_str();

	model_print("%-4d %-2d   %-14s  %7s  %14p   %-#18" PRIx64 " %-2d",
							seq_number, id_to_int(tid), type_str, mo_str, location, get_value(), getOpSize());
	if (is_read()) {
		model_print(" %" PRIx64 " ", get_read_value());
	}
	if (position) {
		model_print("%s", position);
	}
	if (cv) {
		if (is_read())
			model_print(" ");
		else
			model_print("      ");
		cv->print();
	} else
		model_print("\n");
}

/** @brief Get a (likely) unique hash for this ModelAction */
unsigned int ModelAction::hash() const
{
	unsigned int hash = (unsigned int)this->type;
	hash ^= ((unsigned int)this->order) << 3;
	hash ^= seq_number << 5;
	hash ^= id_to_int(tid) << 6;

	return hash;
}

/**
 * Only valid for LOCK, TRY_LOCK, UNLOCK, and WAIT operations.
 * @return The mutex operated on by this action, if any; otherwise NULL
 */
pmc::mutex * ModelAction::get_mutex() const
{
	if (is_trylock() || is_lock() || is_unlock())
		return (pmc::mutex *)get_location();
	else if (is_wait())
		return (pmc::mutex *)get_value();
	else
		return NULL;
}

/** @file execution.h
 *  @brief Model-checker core
 */

#ifndef __EXECUTION_H__
#define __EXECUTION_H__

#include <cstddef>
#include <inttypes.h>

#include "mymemory.h"
#include "hashtable.h"
#include "config.h"
#include "modeltypes.h"
#include "stl-model.h"
#include "params.h"
#include "mypthread.h"
#include <condition_variable>
#include "classlist.h"
#include "mutex.h"
#include "actionlist.h"

/** @brief The central structure for model-checking */
class ModelExecution {
public:
	ModelExecution(ModelChecker *m, Scheduler *scheduler);
	~ModelExecution();

	struct model_params * get_params() const { return params; }
	void setParams(struct model_params * _params) {params = _params;}

	Thread * take_step(ModelAction *curr);

	void print_summary();
	void print_tail();
	void add_thread(Thread *t);
	Thread * get_thread(thread_id_t tid) const;
	Thread * get_thread(const ModelAction *act) const;

	uint32_t get_pthread_counter() { return pthread_counter; }
	void incr_pthread_counter() { pthread_counter++; }
	Thread * get_pthread(pthread_t pid);

	bool is_enabled(Thread *t) const;
	bool is_enabled(thread_id_t tid) const;

	thread_id_t get_next_id();
	unsigned int get_num_threads() const;

	ClockVector * get_cv(thread_id_t tid) const;
	ModelAction * get_parent_action(thread_id_t tid) const;

	ModelAction * get_last_action(thread_id_t tid) const;

	bool check_action_enabled(ModelAction *curr);

	void assert_bug(const char *msg);
	void add_warning(const char *msg, ...);

	bool have_bug_reports() const;

	SnapVector<bug_message *> * get_bugs() const;
	SnapVector<bug_message *> * get_warnings() const;

	bool has_asserted() const;
	void set_assert();
	bool is_complete_execution() const;

	bool is_deadlocked() const;

	Fuzzer * getFuzzer();
	HashTable<pthread_cond_t *, pmc::snapcondition_variable *, uintptr_t, 4> * getCondMap() {return &cond_map;}
	HashTable<pthread_mutex_t *, pmc::snapmutex *, uintptr_t, 4> * getMutexMap() {return &mutex_map;}
	ModelAction * check_current_action(ModelAction *curr);

	bool isFinished() {return isfinished;}
	void setFinished() {isfinished = true;}
	void restore_last_seq_num();
	modelclock_t get_curr_seq_num();
	void remove_action_from_store_buffer(ModelAction *act);
	void add_write_to_lists(ModelAction *act);
	void persistCacheLine(CacheLine *cl, ModelAction *clflush);
	void evictCacheOp(ModelAction *cacheop);

#ifdef TLS
	pthread_key_t getPthreadKey() {return pthreadkey;}
#endif
	SNAPSHOTALLOC
private:
	int get_execution_number() const;
	bool should_wake_up(const ModelAction *curr, const Thread *thread) const;
	void wake_up_sleeping_actions(ModelAction *curr);
	modelclock_t get_next_seq_num();
	bool next_execution();
	void initialize_curr_action(ModelAction *curr);
	void process_read(ModelAction *curr, ModelExecution * exec, ModelAction *rf);
	void handle_read(ModelAction *curr);
	void process_write(ModelAction *curr);
	ModelAction * swap_rmw_write_part(ModelAction *act);
	void process_cache_op(ModelAction *curr);
	void process_memory_fence(ModelAction *curr);
	void process_store_fence(ModelAction *curr);
	bool process_mutex(ModelAction *curr);
	void process_thread_action(ModelAction *curr);
	bool synchronize(const ModelAction *first, ModelAction *second);
	void update_thread_local_data(ModelAction *act);
	void add_normal_write_to_lists(ModelAction *act);
	ModelAction * get_last_unlock(ModelAction *curr) const;
	void build_may_read_from(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rf_set);
	ModelAction * convertNonAtomicStore(void*);

#ifdef TLS
	pthread_key_t pthreadkey;
#endif
	ModelChecker *model;
	struct model_params * params;

	/** The scheduler to use: tracks the running/ready Threads */
	Scheduler * const scheduler;


	SnapVector<Thread *> thread_map;
	SnapVector<Thread *> pthread_map;
	uint32_t pthread_counter;

	action_list_t action_trace;


	/** Per-object list of actions. Maps an object (i.e., memory location)
	 * to a trace of all actions performed on the object.
	 * Used only for unlocks, & wait.
	 */
	HashTable<const void *, simple_action_list_t *, uintptr_t, 2> obj_map;

/** Per-object list of actions. Maps an object (i.e., memory location)
 * to a trace of all actions performed on the object. */
	HashTable<const void *, simple_action_list_t *, uintptr_t, 2> condvar_waiters_map;

	/** Per-object list of writes. These writes are available to all threads */
	HashTable<const void *, simple_action_list_t *, uintptr_t, 2> obj_wr_map;

	HashTable<pthread_mutex_t *, pmc::snapmutex *, uintptr_t, 4> mutex_map;
	HashTable<pthread_cond_t *, pmc::snapcondition_variable *, uintptr_t, 4> cond_map;

	SnapVector<ModelAction *> thrd_last_action;

	HashTable<uintptr_t, CacheLine*, uintptr_t, 6> obj_to_cacheline;
	CacheLine* getCacheLine(void * address);

	/** A special model-checker Thread; used for associating with
	 *  model-checker-related ModelAcitons */
	Thread *model_thread;
	/** Private data members that should be snapshotted. They are grouped
	 * together for efficiency and maintainability. */
	struct model_snapshot_members * const priv;

	Fuzzer * fuzzer;

	Thread * action_select_next_thread(const ModelAction *curr) const;
	bool paused_by_fuzzer(const ModelAction * act) const;
	bool isfinished;

};

#endif	/* __EXECUTION_H__ */

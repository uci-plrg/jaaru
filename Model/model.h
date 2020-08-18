/** @file model.h
 *  @brief Core model checker.
 */

#ifndef __MODEL_H__
#define __MODEL_H__

#include <cstddef>
#include <inttypes.h>
#include <strings.h>

#include "mymemory.h"
#include "hashtable.h"
#include "config.h"
#include "modeltypes.h"
#include "stl-model.h"
#include "context.h"
#include "params.h"
#include "classlist.h"
#include "snapshot-interface.h"
#include "reporter.h"

class Execution_Context {
public:
	Execution_Context(Execution_Context *mprev, ModelExecution *mexecution, NodeStack *mnodestack) :
		prevContext(mprev),
		execution(mexecution),
		nodestack(mnodestack)
	{}

	Execution_Context *prevContext;
	ModelExecution *execution;
	NodeStack *nodestack;

	MEMALLOC
};

/** @brief The central structure for model-checking */
class ModelChecker {
public:
	ModelChecker();
	~ModelChecker();
	model_params * getParams();
	void run();
	void doCrash();

	/** Exit the model checker, intended for pluggins. */
	void exit_model_checker();

	/** @returns the context for the main model-checking system thread */
	ucontext_t * get_system_context() { return &system_context; }

	ModelExecution * get_execution() const { return execution; }
	ModelExecution * getOrigExecution() const { return origExecution; }

	uint get_execution_number() const { return execution_number; }

	Thread * get_thread(thread_id_t tid) const;
	Thread * get_thread(const ModelAction *act) const;

	Thread * get_current_thread() const;

	void switch_from_master(Thread *thread);
	uint64_t switch_to_master(ModelAction *act);

	void assert_bug(const char *msg, ...);

	void assert_user_bug(const char *msg);

	model_params params;
	void startChecker();
	Thread * getInitThread() {return init_thread;}
	Scheduler * getScheduler() {return scheduler;}
	Execution_Context * getPrevContext() {return prevContext;}
	NodeStack * getNodeStack() {return nodestack;}
	void * getRegion(uint ID);
	uint getNextRegionID() { return regionID.size() + 1;}
	void setRegion(uint ID, void *ptr);
	uint getNumCrashes() {return numcrashes;}

	MEMALLOC
private:
	/** Snapshot id we return to restart. */
	snapshot_id snapshot;

	/** The scheduler to use: tracks the running/ready Threads */
	Scheduler * scheduler;
	ModelExecution *execution;
	ModelExecution *origExecution;
	NodeStack *nodestack;
	Thread * init_thread;

	Execution_Context * prevContext;
	uint execution_number;
	uint numcrashes;
	ModelVector<void *> regionID;
	ModelList<NodeStack *> replaystack;

	unsigned int get_num_threads() const;
	bool next_execution();
	bool should_terminate_execution();

	Thread * get_next_thread();
	void reset_to_initial_state();

	ucontext_t system_context;
	double totalstates;
	int totalexplorepoints;
	/** @brief The cumulative execution stats */
	struct execution_stats stats;
	void record_stats();
	void print_bugs() const;
	void print_execution(bool printbugs) const;
	void print_stats() const;
};

extern ModelChecker *model;
extern int inside_model;
extern int FILLBYTE;
void createModelIfNotExist();
void parse_options(struct model_params *params);
void createModelIfNotExist();

#endif	/* __MODEL_H__ */

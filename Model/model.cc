#include <stdio.h>
#include <algorithm>
#include <new>
#include <stdarg.h>
#include <string.h>
#include <cstdlib>
#include <dlfcn.h>
#include <signal.h>

#include "model.h"
#include "action.h"
#include "schedule.h"
#include "snapshot-interface.h"
#include "common.h"
#include "threads-model.h"
#include "output.h"
#include "execution.h"
#include "params.h"
#include "datarace.h"
#include "nodestack.h"
#include "threadmemory.h"
#include "config.h"
#include "pminterface.h"
#include "persistentmemory.h"

ModelChecker *model = NULL;
int inside_model = 0;

void placeholder(void *) {
	ASSERT(0);
}

static void mprot_handle_pf(int sig, siginfo_t *si, void *unused)
{
	model_print("Segmentation fault at %p\n", si->si_addr);
	model_print("For debugging, place breakpoint at: %s:%d\n",__FILE__, __LINE__);
	model_print("Or attach gdb to process with id # %u\n", getpid());
	print_trace();	// Trace printing may cause dynamic memory allocation
	while(1)
		;
}

void install_handler() {
	stack_t ss;
	ss.ss_sp = model_malloc(SIGSTACKSIZE);
	ss.ss_size = SIGSTACKSIZE;
	ss.ss_flags = 0;
	sigaltstack(&ss, NULL);
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_RESTART | SA_ONSTACK;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = mprot_handle_pf;

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		perror("sigaction(SIGSEGV)");
		exit(EXIT_FAILURE);
	}

}

void * getRegionFromID(uint ID) {
	if (!model) {
		inside_model = 1;
		snapshot_system_init(10000, 1024, 1024, 40000);
		model = new ModelChecker();
		model->startChecker();
		inside_model = 0;
	}

	return model->getRegion(ID);
}


void setRegionFromID(uint ID, void *ptr) {
	if (!model) {
		inside_model = 1;
		snapshot_system_init(10000, 1024, 1024, 40000);
		model = new ModelChecker();
		model->startChecker();
		inside_model = 0;
	}

	model->setRegion(ID, ptr);
}

void * ModelChecker::getRegion(uint ID) {
	if (ID >= regionID.size())
		return NULL;
	else
		return regionID[ID];
}

void ModelChecker::setRegion(uint ID, void *ptr) {
	regionID.setExpand(ID, ptr);
}

/** @brief Constructor */
ModelChecker::ModelChecker() :
	/* Initialize default scheduler */
	params(),
	scheduler(new Scheduler()),
	execution(new ModelExecution(this, scheduler)),
	origExecution(execution),
	nodestack(new NodeStack()),
	prevContext(NULL),
	execution_number(1),
	numcrashes(0),
	replaystack()
{
	model_print("PMCheck\n"
							"Copyright (c) 2019 Regents of the University of California. All rights reserved.\n"
							"Written by Hamed Gorjiara, Brian Demsky, Peizhao Ou, Brian Norris, and Weiyu Luo\n\n");
	memset(&stats,0,sizeof(struct execution_stats));
	init_thread = new Thread(execution->get_next_id(), (thrd_t *) model_malloc(sizeof(thrd_t)), &placeholder, NULL, NULL);
#ifdef TLS
	init_thread->setTLS((char *)get_tls_addr());
#endif
	execution->add_thread(init_thread);
	scheduler->set_current_thread(init_thread);
	execution->setParams(&params);
	param_defaults(&params);
	parse_options(&params);
	initRaceDetector();
	/* Configure output redirection for the model-checker */
	install_handler();
}

/** @brief Destructor */
ModelChecker::~ModelChecker()
{
	delete scheduler;
}

/** Method to set parameters */
model_params * ModelChecker::getParams() {
	return &params;
}

/**
 * Restores user program to initial state and resets all model-checker data
 * structures.
 */
void ModelChecker::reset_to_initial_state() {
	snapshot_roll_back(snapshot);
}

/** @return the number of user threads created during this execution */
unsigned int ModelChecker::get_num_threads() const
{
	return execution->get_num_threads();
}

/**
 * Must be called from user-thread context (e.g., through the global
 * thread_current() interface)
 *
 * @return The currently executing Thread.
 */
Thread * ModelChecker::get_current_thread() const
{
	return scheduler->get_current_thread();
}

/**
 * @brief Choose the next thread to execute.
 *
 * This function chooses the next thread that should execute. It can enforce
 * execution replay/backtracking or, if the model-checker has no preference
 * regarding the next thread (i.e., when exploring a new execution ordering),
 * we defer to the scheduler.
 *
 * @return The next chosen thread to run, if any exist. Or else if the current
 * execution should terminate, return NULL.
 */
Thread * ModelChecker::get_next_thread()
{
	/*
	 * Have we completed exploring the preselected path? Then let the
	 * scheduler decide
	 */
	return scheduler->select_next_thread();
}

/**
 * @brief Assert a bug in the executing program.
 *
 * Use this function to assert any sort of bug in the user program. If the
 * current trace is feasible (actually, a prefix of some feasible execution),
 * then this execution will be aborted, printing the appropriate message. If
 * the current trace is not yet feasible, the error message will be stashed and
 * printed if the execution ever becomes feasible.
 *
 * @param msg Descriptive message for the bug (do not include newline char)
 * @return True if bug is immediately-feasible
 */
void ModelChecker::assert_bug(const char *msg, ...)
{
	char str[800];

	va_list ap;
	va_start(ap, msg);
	vsnprintf(str, sizeof(str), msg, ap);
	va_end(ap);

	execution->assert_bug(str);
}

/**
 * @brief Assert a bug in the executing program, asserted by a user thread
 * @see ModelChecker::assert_bug
 * @param msg Descriptive message for the bug (do not include newline char)
 */
void ModelChecker::assert_user_bug(const char *msg)
{
	/* If feasible bug, bail out now */
	assert_bug(msg);
	switch_to_master(NULL);
}

/** @brief Print bug report listing for this execution (if any bugs exist) */
void ModelChecker::print_bugs() const
{
	ModelVector<bug_message *> *warnings = execution->get_warnings();

	model_print("Warning report: %zu warning%s detected\n",
							warnings->size(),
							warnings->size() > 1 ? "s" : "");
	for (unsigned int i = 0;i < warnings->size();i++)
		(*warnings)[i] -> print();

	ModelVector<bug_message *> *bugs = execution->get_bugs();

	model_print("Bug report: %zu bug%s detected\n",
							bugs->size(),
							bugs->size() > 1 ? "s" : "");
	for (unsigned int i = 0;i < bugs->size();i ++)
		(*bugs)[i] -> print();
}

/**
 * @brief Record end-of-execution stats
 *
 * Must be run when exiting an execution. Records various stats.
 * @see struct execution_stats
 */
void ModelChecker::record_stats()
{
	stats.num_total ++;
	if (execution->have_bug_reports())
		stats.num_buggy_executions ++;
	else if (execution->is_complete_execution())
		stats.num_complete ++;
	else {
		//All threads are sleeping
		/**
		 * @todo We can violate this ASSERT() when fairness/sleep sets
		 * conflict to cause an execution to terminate, e.g. with:
		 * Scheduler: [0: disabled][1: disabled][2: sleep][3: current, enabled]
		 */
		//ASSERT(scheduler->all_threads_sleeping());
	}
}

/** @brief Print execution stats */
void ModelChecker::print_stats() const
{
	model_print("Number of complete, bug-free executions: %d\n", stats.num_complete);
	model_print("Number of buggy executions: %d\n", stats.num_buggy_executions);
	model_print("Total executions: %d\n", stats.num_total);
}

/**
 * @brief End-of-exeuction print
 * @param printbugs Should any existing bugs be printed?
 */
void ModelChecker::print_execution(bool printbugs) const
{
	model_print("Program output from execution %d:\n",
							get_execution_number());
	print_program_output();

	if (params.verbose >= 3) {
		print_stats();
	}

	/* Don't print invalid bugs */
	if (printbugs && execution->have_bug_reports()) {
		model_print("\n");
		print_bugs();
	}

	model_print("\n");
	execution->print_summary();
}

/**
 * Queries the model-checker for more executions to explore and, if one
 * exists, resets the model-checker state to execute a new execution.
 *
 * @return If there are more executions to explore, return true. Otherwise,
 * return false.
 */
bool ModelChecker::next_execution() {
	DBG();
	/* Is this execution a feasible execution that's worth bug-checking? */
	bool complete = (execution->is_complete_execution() ||
									 execution->have_bug_reports());

	/* End-of-execution bug checks */
	if (complete) {
		if (execution->is_deadlocked())
			assert_bug("Deadlock detected");
	}

	record_stats();
	/* Output */
	if ( (complete && params.verbose) || params.verbose>1 || (complete && execution->have_bug_reports()))
		print_execution(complete);
	else
		clear_program_output();

	execution_number ++;

	//See if we are done...
	if (!nodestack->has_another_execution()) {
		//last execution on this stack...need to reset
		if (prevContext == NULL) {
			//really done
			return false;
		}
		execution->clearPreRollback();

		//Delete non-snapshotting data structures
		delete nodestack;
		delete execution;

		//Restore previous execution information
		execution  = prevContext->execution;
		nodestack = prevContext->nodestack;

		Execution_Context * tmp = prevContext->prevContext;
		delete prevContext;
		prevContext = tmp;
	}

	while (prevContext != NULL) {
		delete execution;
		nodestack->reset_execution();
		replaystack.push_front(nodestack);
		execution = prevContext->execution;
		nodestack = prevContext->nodestack;
		Execution_Context * tmp = prevContext->prevContext;
		delete prevContext;
		prevContext = tmp;
	}

	//Reset nodestack for next execution
	nodestack->reset_execution();

	//If not the top of the stack we do a replay
	if (!replaystack.empty())
		nodestack->repeat_prev_execution();

	//Delete old execution
	delete execution;

	//Reset program state
	reset_to_initial_state();

	//Build new execution
	execution = new ModelExecution(this, scheduler);
	execution->get_next_id();	//increment thread count for init_thread in execution object since it is a new object
	execution->add_thread(init_thread);
	execution->setParams(&params);

	//We need to reset the original execution also and reset the persistent memory
	origExecution = execution;
	initializePersistentMemory();
	regionID.clear();
	numcrashes = 0;

	return true;
}

/**
 * @brief Get a Thread reference by its ID
 * @param tid The Thread's ID
 * @return A Thread reference
 */
Thread * ModelChecker::get_thread(thread_id_t tid) const {
	return execution->get_thread(tid);
}

/**
 * @brief Get a reference to the Thread in which a ModelAction was executed
 * @param act The ModelAction
 * @return A Thread reference
 */
Thread * ModelChecker::get_thread(const ModelAction *act) const {
	return execution->get_thread(act);
}

/**
 * Switch from a model-checker context to a user-thread context. This is the
 * complement of ModelChecker::switch_to_master and must be called from the
 * model-checker context
 *
 * @param thread The user-thread to switch to
 */
void ModelChecker::switch_from_master(Thread *thread) {
	scheduler->set_current_thread(thread);
	inside_model = 0;
	Thread::swap(&system_context, thread);
}

/**
 * Switch from a user-context to the "master thread" context (a.k.a. system
 * context). This switch is made with the intention of exploring a particular
 * model-checking action (described by a ModelAction object). Must be called
 * from a user-thread context.
 *
 * @param act The current action that will be explored. May be NULL only if
 * trace is exiting via an assertion (see ModelExecution::set_assert and
 * ModelExecution::has_asserted).
 * @return Return the value returned by the current action
 */
uint64_t ModelChecker::switch_to_master(ModelAction *act) {
	if (modellock) {
		static bool fork_message_printed = false;

		if (!fork_message_printed) {
			model_print("Fork handler or dead thread trying to call into model checker...\n");
			fork_message_printed = true;
			ASSERT(0);
		}
		delete act;
		return 0;
	}
	DBG();
	Thread *old = thread_current();
	scheduler->set_current_thread(NULL);
	ASSERT(!old->get_pending());

	inside_model = 1;
	old->set_pending(act);
	if (Thread::swap(old, &system_context) < 0) {
		perror("swap threads");
		exit(EXIT_FAILURE);
	}
	return old->get_return_value();
}

static void runChecker() {
	model->run();
	delete model;
}

void ModelChecker::startChecker() {
	startExecution(get_system_context(), runChecker);
	snapshot = take_snapshot();
	redirect_output();
	initMainThread();
}

bool ModelChecker::should_terminate_execution()
{
	if (execution->have_bug_reports()) {
		execution->set_assert();
		return true;
	} else if (execution->isFinished()) {
		return true;
	}
	return false;
}

void ModelChecker::doCrash() {
	model_print("Execution %d at sequence number %d\n",execution_number, execution->get_curr_seq_num());
	Execution_Context * ec = new Execution_Context(prevContext, execution, nodestack);
	prevContext = ec;
	execution->clearPreRollback();
	reset_to_initial_state();
	execution = new ModelExecution(this, scheduler);
	if (replaystack.empty())
		nodestack = new NodeStack();
	else {
		nodestack = replaystack.pop_front();
		if (!replaystack.empty())
			nodestack->repeat_prev_execution();
	}
	execution->get_next_id();	//increment thread count for init_thread in execution object since it is a new object
	execution->add_thread(init_thread);
	execution->setParams(&params);
	numcrashes++;
}

/** @brief Run ModelChecker for the user program */
void ModelChecker::run()
{
	do {
		Thread * t = init_thread;
nextExecution:
		if (params.pmdebug && prevContext != NULL) {
			model_print("\n\n*******************************************************************\n");
			model_print("Execution %u\n", get_execution_number());
			model_print("*******************************************************************\n\n");
		}

		do {
			/*
			 * Stash next pending action(s) for thread(s). There
			 * should only need to stash one thread's action--the
			 * thread which just took a step--plus the first step
			 * for any newly-created thread
			 */
			for (unsigned int i = 0;i < get_num_threads();i++) {
				thread_id_t tid = int_to_id(i);
				Thread *thr = get_thread(tid);
				if (!thr->is_model_thread() && !thr->is_complete() && !thr->get_pending()) {
					switch_from_master(thr);
					ModelAction *pendact = thr->get_pending();
					if (pendact!= NULL && (pendact->is_write() || pendact->is_read())) {
						execution->ensureInitialValue(pendact);
					}
					if (thr->is_waiting_on(thr))
						assert_bug("Deadlock detected (thread %u)", i);
				}
			}

			if (execution->getStoreBuffer() > params.storebufferthreshold * execution->get_num_threads()) {
				uint targetthread = random() % execution->get_num_threads();
				uint numberEvict = random() % params.evictmax;
				ThreadMemory * mem = get_thread(int_to_id(targetthread))->getMemory();
				for(uint i=0;i<numberEvict;i++) {
					if (mem->popFromStoreBuffer()) {
						doCrash();
						goto nextExecution;
					}
				}
			}


			/* Don't schedule threads which should be disabled */
			for (unsigned int i = 0;i < get_num_threads();i++) {
				Thread *th = get_thread(int_to_id(i));
				ModelAction *act = th->get_pending();
				if (act && execution->is_enabled(th) && !execution->check_action_enabled(act)) {
					scheduler->sleep(th);
				}
			}

			for (unsigned int i = 1;i < get_num_threads();i++) {
				Thread *th = get_thread(int_to_id(i));
				ModelAction *act = th->get_pending();
				if (act && execution->is_enabled(th) && (th->get_state() != THREAD_BLOCKED) ) {
					if (act->get_type() == THREAD_CREATE || \
							act->get_type() == PTHREAD_CREATE || \
							act->get_type() == THREAD_START || \
							act->get_type() == THREAD_FINISH) {
						t = th;
						break;
					}
				}
			}

			/* Catch assertions from prior take_step or from
			* between-ModelAction bugs (e.g., data races) */

			if (execution->has_asserted())
				break;
			if (!t)
				t = get_next_thread();
			if (!t || t->is_model_thread())
				break;
			if (t->just_woken_up()) {
				t->set_wakeup_state(false);
				t->set_pending(NULL);
				t = NULL;
				continue;	// Allow this thread to stash the next pending action
			}

			/* Consume the next action for a Thread */
			ModelAction *curr = t->get_pending();

			t->set_pending(NULL);
			t = execution->take_step(curr);
			if (execution->getCrashed()) {
				doCrash();
				goto nextExecution;
			}
		} while (!should_terminate_execution());
	} while(next_execution());
	model_print("******* Model-checking complete: *******\n");
	print_stats();

	/* unlink tmp file created by last child process */
	char filename[256];
	snprintf_(filename, sizeof(filename), "PMCheckOutput%d", getpid());
	unlink(filename);
}



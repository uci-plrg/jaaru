#ifndef __REPORTER_H__
#define __REPORTER_H__

#include "common.h"
#include "mymemory.h"

struct bug_message {
	bug_message(const char *str) {
		const char *fmt = "  [BUG] %s\n";
		msg = (char *)snapshot_malloc(strlen(fmt) + strlen(str));
		sprintf(msg, fmt, str);
	}
	~bug_message() { if (msg) snapshot_free(msg);}

	char *msg;
	void print() { model_print("%s", msg); }

	SNAPSHOTALLOC
};

/** @brief Model checker execution stats */
struct execution_stats {
        int num_total;  /**< @brief Total number of executions */
        int num_buggy_executions;       /** @brief Number of buggy executions */
        int num_complete;       /**< @brief Number of feasible, non-buggy, complete executions */
};

#endif	/* __BUGMESSAGE_H__ */

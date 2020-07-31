#ifndef __PARAMS_H__
#define __PARAMS_H__

/**
 * Model checker parameter structure. Holds run-time configuration options for
 * the model checker.
 */
struct model_params {
	int maxexecutions;
	bool nofork;
	modelclock_t traceminsize;
	uint storebufferthreshold;
	uint evictmax;
	int verbose;
};

void param_defaults(struct model_params *params);

#endif	/* __PARAMS_H__ */

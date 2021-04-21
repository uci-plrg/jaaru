/** @file main.cc
 *  @brief Entry point for the model checker.
 */

#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "common.h"
#include "output.h"
#include "plugins.h"
/* global "model" object */
#include "model.h"
#include "params.h"
#include "snapshot-interface.h"

void param_defaults(struct model_params *params)
{
	params->firstCrash = 0;
	params->verbose = !!DBG_ENABLED();
	params->nofork = false;
	params->evictmax = 10;
	params->storebufferthreshold = 15;
	params->pmdebug = 0;
	params->printSpace = false;
	params->dumpStack = false;
	params->numcrashes = 1;
	params->file = NULL;
	params->enableCrash = true;
	params->enablePersistrace = false;
	params->randomExecution = -1;
}


static void print_usage(struct model_params *params)
{
	/* Reset defaults before printing */
	param_defaults(params);

	model_print(
		"Copyright (c) 2021 Regents of the University of California. All rights reserved.\n"
		"Written by Hamed Gorjiara, Brian Demsky, Peizhao Ou, Brian Norris, and Weiyu Luo\n"
		"\n"
		"Usage: PMCheck=[MODEL-CHECKER OPTIONS]\n"
		"\n"
		"MODEL-CHECKER OPTIONS can be any of the model-checker options listed below. Arguments\n"
		"provided after the `--' (the PROGRAM ARGS) are passed to the user program.\n"
		"\n"
		"Model-checker options:\n"
		"-h, --help                  Display this help message and exit\n"
		"-v[NUM], --verbose[=NUM]    Print verbose execution information. NUM is optional:\n"
		"                              0 is quiet; 1 shows valid executions; 2 is noisy;\n"
		"                              3 is noisier.\n"
		"                              Default: %d\n"
		"-p                          PMDebug level\n"
		"-t                          Dump Stack at Crash Injection\n"
		"-f                          Memory initialization byte\n"
		"-r                          model clock for first possible crash\n"
		"-n                          No fork\n"
		"-s                          Print size of exploration space\n"
		"-c                          Number of nested crashes\n"
		"                            Default: %u\n"
		"-d [file]					 Deleting the persistent file after each execution.\n"
		"-e							 Enable manual crash point.\n"
		"-x							 Enable random execution (default execution number = 30)\n"
		"-y							 Enable Persistency race analysis\n",
		params->verbose, params->numcrashes);
	exit(EXIT_SUCCESS);
}

void parse_options(struct model_params *params) {
	const char *shortopts = "hsetnyv::x::p::r:d::c:f:";
	const struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", optional_argument, NULL, 'v'},
		{0, 0, 0, 0}	/* Terminator */
	};
	int opt, longindex;
	bool error = false;
	char * options = getenv("PMCheck");

	if (options == NULL)
		return;
	int argc = 1;
	int index;
	for(index = 0;options[index]!=0;index++) {
		if (options[index] == ' ')
			argc++;
	}
	argc++;	//first parameter is executable name
	char optcpy[index + 1];
	memcpy(optcpy, options, index+1);
	char * argv[argc + 1];
	argv[0] = NULL;
	argv[1] = optcpy;
	int count = 2;
	for(index = 0;optcpy[index]!=0;index++) {
		if (optcpy[index] == ' ') {
			argv[count++] = &optcpy[index+1];
			optcpy[index] = 0;
		}
	}

	while (!error && (opt = getopt_long(argc, argv, shortopts, longopts, &longindex)) != -1) {
		switch (opt) {
		case 'h':
			print_usage(params);
			break;
		case 's':
			params->printSpace = true;
			break;
		case 'n':
			params->nofork = true;
			break;
		case 'e':
			params->enableCrash = false;
			break;
		case 't':
			params->dumpStack = true;
			break;
		case 'v':
			params->verbose = optarg ? atoi(optarg) : 1;
			break;
		case 'p':
			params->pmdebug = optarg ? atoi(optarg) : 1;
			break;
		case 'x':
			params->randomExecution = optarg ? atoi(optarg) : 30;
			break;
		case 'y':
			params->enablePersistrace = true;
			enableAnalysis(PERSISTRACENAME);
			break;
		case 'f':
			FILLBYTE = atoi(optarg);
			break;
		case 'r':
			params->firstCrash = (modelclock_t) atoi(optarg);
			break;
		case 'c':
			params->numcrashes = atoi(optarg);
			break;
		case 'd': {
			size_t len = strlen (optarg) + 1;
			params->file = (char *) model_calloc(len, sizeof(char));
			memcpy (params->file, optarg, len);
			break;
		} default:	/* '?' */
			error = true;
			break;
		}
	}

	/* Special value to reset implementation as described by Linux man page.  */
	optind = 0;

	if (error)
		print_usage(params);
}


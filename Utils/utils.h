#ifndef UTILS_H
#define UTILS_H
#include <sys/time.h>
#include <stdlib.h>

static inline long long current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL);	// get current time
	long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;	// calculate milliseconds
	// printf("milliseconds: %lld\n", milliseconds);
	return milliseconds;
}

#endif
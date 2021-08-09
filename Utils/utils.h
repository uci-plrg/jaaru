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

static inline void replace_char(char *str, char oldchar, char newchar) {
	uint index = 0;
	while(str && str[index] != '\0') {
		if(str[index] == oldchar) {
			str[index] = newchar;
		}
		index++;
	}
}

#endif
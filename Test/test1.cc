#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"

using namespace std;
#define NUMTHREADS 2

atomic<unsigned int> x(0);
atomic<unsigned int> y(0);

void *func1(void *)
{
	x.store(1);
	y.store(3);
	x.store(5);
	cacheOperation(CLWB, (char *)&x, sizeof(unsigned int));
	x.store(9);
	mfence();
	printf("Func1: Value X= %u\n", x.load());
	return NULL;
}


void *func2(void *)
{
	x.store(2);
	y.store(4);
	x.store(6);
	cacheOperation(CLWB, (char*)&x, sizeof(unsigned int));
	x.store(8);
	mfence();
	printf("Func2: Value X= %u\n", x.load());
	return NULL;
}

int main() {
	pthread_t threads[NUMTHREADS];
	void *(*funcptr[])(void *) = {func1, func2};
	for (int i=0;i< NUMTHREADS;i++) {
		if( int retval = pthread_create(&threads[i], NULL, funcptr[i], NULL ) ) {
			fprintf(stderr, "Unable to create a pthread. Return value %d\n", retval);
			return EXIT_FAILURE;
		}
	}

	for(int i=0;i< NUMTHREADS;i++) {
		pthread_join(threads[i], NULL);
	}

	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"

using namespace std;
#define NUMTHREADS 2

atomic<unsigned int> x;
atomic<unsigned int> y;

void *func1(void *){
	pmc_atomic_store32(&x, 1, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 1, 4, (const char *) (&y) );
	pmc_clwb((void *) &x);
	pmc_sfence();
	pmc_atomic_store32(&x, 2, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 2, 4, (const char *) (&y) );
	pmc_clwb((void *) &x);
	pmc_atomic_store32(&x, 3, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 3, 4, (const char *) (&y) );
	pmc_sfence();
	printf("X=%d\n", pmc_atomic_load32(&x, 4, (const char *) &x) );
	return NULL;
}


void *func2(void *){
	pmc_atomic_store32(&x, 100, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 100, 4, (const char *) (&y) );
	pmc_clwb((void *) &x);
	pmc_sfence();
	pmc_atomic_store32(&x, 200, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 200, 4, (const char *) (&y) );
	pmc_clwb((void *) &x);
	pmc_atomic_store32(&x, 300, 4, (const char *) (&x) );
	pmc_atomic_store32(&y, 300, 4, (const char *) (&y) );
	pmc_sfence();
	printf("Y=%d\n", pmc_atomic_load32(&y, 4, (const char *) &y) );
	return NULL;
}

int main() {
	pmc_atomic_init32(&x, 0, (const char *) (&x));
	pmc_atomic_init32(&y, 0, (const char *) (&y));
	pthread_t threads[NUMTHREADS];
	void *(*funcptr[])(void *) = {func1, func2};
	for (int i=0; i< NUMTHREADS; i++){
		if( int retval = pthread_create(&threads[i], NULL, funcptr[i], NULL ) ){
			fprintf(stderr, "Unable to create a pthread. Return value %d\n", retval);
			return EXIT_FAILURE;
		}
	}

	for(int i=0; i< NUMTHREADS; i++){
		pthread_join(threads[i], NULL);
	}

	return EXIT_SUCCESS;
}

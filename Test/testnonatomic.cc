#include <atomic>
#include <pthread.h>
#include <stdio.h>
#include <cstdlib>
#include "test.h"

using namespace std;
#define NUMTHREADS 2

atomic<unsigned int> x(0);
atomic<unsigned int> y(0);

void* func1(void *){
    x=1;
	y=3;
    x.store(5, std::memory_order_release);
    x=10;
	cacheOperation(CLWB, (char *)&x, sizeof(unsigned int));
    cacheOperation(CLWB, (char *)&y, sizeof(unsigned int));
	mfence();
    printf("Func2: Value Y= %u\n", y.load());
    return NULL;
}

void* func2(void *)
{
    while(x.load(std::memory_order_acquire) < 5);
	y.store(2, std::memory_order_relaxed);
    x.store(4, std::memory_order_relaxed);
	cacheOperation(CLWB, (char*)&y, sizeof(unsigned int));
	mfence();
	printf("Func2: Value Y= %u\n", y.load());
    return NULL;
}


int main(){
    pthread_t threads[NUMTHREADS];
    atomic_init<uint>(&x, 19);
    atomic_init<uint>(&y, 20);
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
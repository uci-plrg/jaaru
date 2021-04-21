#include <atomic>
#include <pthread.h>
#include <stdio.h>
#include <cstdlib>
#include "test.h"

using namespace std;
#define NUMTHREADS 2
#define TEST1

extern "C" {
void * getRegionFromID(uint ID);
void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
	testStruct() : a(0), b(0) {}
	uint32_t a;
	uint32_t b;
} testStruct;

void* func1(void * input){
	testStruct *myobj = (testStruct *) input;
	myobj->a = 1;
	cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
	mfence();
#ifdef TEST1
	myobj->a=3;
	myobj->b=4;
	cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
	mfence();
#endif
	return NULL;
}

void* func2(void * input)
{
	testStruct *myobj = (testStruct *) input;
	myobj->b = 2;
	cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
	mfence();
#ifdef TEST1
	myobj->a=30;
	myobj->b=40;
	cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
	mfence();
#endif
	return NULL;
}

int main(){
	testStruct *myobj;
	if (getRegionFromID(0) == NULL)
	{
		myobj = (testStruct *)malloc(sizeof(testStruct) + 64 + 64 + 64);
		//Make sure a,b,x are in the same cache line
		myobj = (testStruct *)(((uintptr_t) myobj & ~(64 - 1)) + 64);
		setRegionFromID(0, myobj);
		pthread_t threads[NUMTHREADS];
		void *(*funcptr[])(void *) = {func1, func2};
		for (int i=0;i< NUMTHREADS;i++) {
			if( int retval = pthread_create(&threads[i], NULL, funcptr[i], (void *) myobj ) ) {
				fprintf(stderr, "Unable to create a pthread. Return value %d\n", retval);
				return EXIT_FAILURE;
			}
		}
		for(int i=0;i< NUMTHREADS;i++) {
			pthread_join(threads[i], NULL);
		}
	} else {
		myobj = (testStruct *)getRegionFromID(0);
		printf("a=%u\n", myobj->a);
		printf("b=%u\n", myobj->b);
	}
	return EXIT_SUCCESS;
}
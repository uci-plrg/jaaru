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
	testStruct() : x(0), y(0), z(0), var(0) {}
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t var;
} testStruct;

void* func1(void * input){
	testStruct *myobj = (testStruct *) input;
	myobj->x = 1;
	myobj->x=2;
	myobj->y=3;
	myobj->x=3;
	myobj->var=11;
	myobj->x=4;
#ifdef TEST1
	cacheOperation(CLFLUSH, (char *)myobj, sizeof(myobj));
#endif
	return NULL;
}

void* func2(void * input)
{
	testStruct *myobj = (testStruct *) input;
	myobj->y = 10;
	myobj->x=50;
	myobj->y=20;
	myobj->z=10;
	myobj->x=60;
#ifdef TEST1
	cacheOperation(CLFLUSH, (char *)myobj, sizeof(myobj));
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
		printf("x=%u\n", myobj->x);
		printf("y=%u\n", myobj->y);
		printf("z=%u\n", myobj->z);
		printf("var=%u\n", myobj->var);
	}
	return EXIT_SUCCESS;
}
#include <stdlib.h>
#include <stdio.h>
#include "test.h"

//#define TEST1
//#define TEST2
//#define MULTICACHELINE
extern "C" {
void * getRegionFromID(uint ID);
void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
	uint32_t a = 0;
#ifdef TEST1
	char nope [64];
#endif
	uint32_t b = 0;
} testStruct;

int main()
{
	testStruct *myobj;
	if (getRegionFromID(0) == NULL)
	{
		myobj = (testStruct *)malloc(sizeof(testStruct) + 64 + 64 + 64);
#ifdef MULTICACHELINE
		myobj = (testStruct *)(((uintptr_t) myobj & ~(64 - 1)) + 64 + 60);
#else
		myobj = (testStruct *)(((uintptr_t) myobj & ~(64 - 1)) + 64);
#endif
		setRegionFromID(0, myobj);
		myobj->a = 1;
#ifdef TEST2
		cacheOperation(CLFLUSH, (char *)myobj, sizeof(myobj));
#else
		cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
#endif
		myobj->b = 2;
		mfence();
	}
	else
	{
		myobj = (testStruct *)getRegionFromID(0);
		printf("a=%u\tb=%u\n", myobj->a, myobj->b);
	}
	return EXIT_SUCCESS;
}
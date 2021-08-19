#include <stdlib.h>
#include <stdio.h>
#include "test.h"

extern "C" {
void * getRegionFromID(uint ID);
void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
	uint64_t a = 0;
	uint64_t b = 0;
} testStruct;

int main()
{
	testStruct *myobj;
	if (getRegionFromID(0) == NULL)
	{
		myobj = (testStruct *)malloc(sizeof(testStruct) + 64 + 64 + 64);
		myobj = (testStruct *)(((uintptr_t) myobj & ~(64 - 1)) + 64);
		setRegionFromID(0, myobj);
		testStruct tmpobj = {
			.a = 1,
			.b = 2,
		};
		*myobj = tmpobj;
		cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
		mfence();
	}
	else
	{
		myobj = (testStruct *)getRegionFromID(0);
		printf("a=%lu\tb=%lu\n", myobj->a, myobj->b);
	}
	return EXIT_SUCCESS;
}
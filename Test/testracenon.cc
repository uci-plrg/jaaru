#include <stdlib.h>
#include <stdio.h>
#include <atomic>
#include "test.h"

#define TEST1

extern "C" {
void * getRegionFromID(uint ID);
void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
	testStruct() : a(0), b(0), x(0) {}
	atomic<uint32_t> a;
	atomic<uint32_t> b;
#ifdef TEST1
	char nope [64];
#endif
	atomic<uint32_t> x;
} testStruct;

int main()
{
	testStruct *myobj;
	if (getRegionFromID(0) == NULL)
	{
		myobj = (testStruct *)malloc(sizeof(testStruct) + 64 + 64 + 64);
		//Make sure a,b,x are in the same cache line
		myobj = (testStruct *)(((uintptr_t) myobj & ~(64 - 1)) + 64);
		setRegionFromID(0, myobj);
		myobj->a.store(1, std::memory_order_relaxed);
		cacheOperation(CLFLUSH, (char *)myobj, sizeof(myobj));
		myobj->b.store(2, std::memory_order_acq_rel);
		myobj->x.store(5, std::memory_order_release);
		mfence();
	}
	else
	{
		myobj = (testStruct *)getRegionFromID(0);
		printf("atomic x = %d\n", myobj->x.load());
		printf("a=%u\n", myobj->a.load());
	}
	return EXIT_SUCCESS;
}
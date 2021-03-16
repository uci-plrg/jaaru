#include<stdlib.h>
#include<stdio.h>
#include<atomic>
#include "test.h"

extern "C" {
    void * getRegionFromID(uint ID);
    void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
    testStruct(): a(0), b(0), x(0) {}
    uint32_t a;
    uint32_t b;
    char nope [64];
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
        myobj->a = 1;
        cacheOperation(CLFLUSHOPT, (char *)myobj, sizeof(myobj));        
        myobj->b = 2;
		myobj->x.store(5, std::memory_order_release);
        mfence();
    }
    else
    {
        myobj = (testStruct *)getRegionFromID(0);
		printf("atomic x = %d\n", myobj->x.load());
        printf("a=%u\n", myobj->a);
    }
    return EXIT_SUCCESS;
}
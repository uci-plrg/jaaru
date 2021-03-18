#include<stdlib.h>
#include<stdio.h>
#include<atomic>
#include "test.h"

//#define TEST1
#define TEST2
#define TEST3

extern "C" {
    void * getRegionFromID(uint ID);
    void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
    testStruct(): a(0), b(0), x(0) {}
    uint32_t a;
    uint32_t b;
#ifdef TEST1
    char nope [128];
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
        myobj->a = 1;
        cacheOperation(CLWB, (char *)myobj, sizeof(myobj));        
		myobj->x.store(5, std::memory_order_release);
        myobj->b = 2;
#ifdef TEST2
        cacheOperation(CLWB, (char *)myobj, sizeof(myobj));        
#endif
        mfence();
#ifdef TEST3
        myobj->a=3;
        myobj->b=4;
        cacheOperation(CLWB, (char *)myobj, sizeof(myobj));
        mfence();
#endif
    }
    else
    {
        myobj = (testStruct *)getRegionFromID(0);
		printf("atomic x = %d\n", myobj->x.load());
        printf("a=%u\n", myobj->a);
#if defined(TEST2) || defined(TEST3)
        printf("b=%u\n", myobj->b);
#endif
    }
    return EXIT_SUCCESS;
}
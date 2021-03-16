#include<stdlib.h>
#include<stdio.h>
#include "test.h"

extern "C" {
    void * getRegionFromID(uint ID);
    void setRegionFromID(uint ID, void *ptr);
}

typedef struct testStruct
{
    uint32_t a = 0;
    //char nope [64];
    uint32_t b = 0;
} testStruct;

int main()
{
    testStruct *myobj;
    if (getRegionFromID(0) == NULL)
    {
        myobj = (testStruct *)malloc(sizeof(testStruct) + 192);
        setRegionFromID(0, myobj);
        myobj->a = 1;
        cacheOperation(CLFLUSHOPT, (char *)myobj, sizeof(myobj));        
        myobj->b = 2;
        mfence();
        free(myobj);
    }
    else
    {
        myobj = (testStruct *)getRegionFromID(0);
        printf("a=%u\tb=%u\n", myobj->a, myobj->b);
    }
    return EXIT_SUCCESS;
}
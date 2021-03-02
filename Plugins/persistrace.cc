#include "persistrace.h"

unsigned int hashCacheLineKey(CacheLineMetaData *clm) {
    return ((uintptr_t)clm->execution >> 4) ^ ((uintptr_t)clm->cacheID >> 6);
}

bool equalCacheLineKey(CacheLineMetaData *clm1, CacheLineMetaData *clm2) {
    return (clm1 && clm2 && clm1->execution == clm2->execution && clm1->cacheID == clm2->cacheID) || (clm1 == clm2 && clm1 == NULL);
}
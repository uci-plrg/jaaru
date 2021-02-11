#include "cacheline.h"
#include "common.h"
#include "action.h"
#include "model.h"
#include "execution.h"


CacheLine::CacheLine(void *address) :
	Range(),
	id(getCacheID(address)),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

CacheLine::CacheLine(uintptr_t _id) :
	Range(),
	id(_id),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

#include "cacheline.h"
#include "common.h"
#include "action.h"
#include "model.h"
#include "execution.h"


CacheLine::CacheLine(void *address) :
	id(getCacheID(address)),
	beginR(0),
	endR(0),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

CacheLine::CacheLine(uintptr_t _id) :
	id(_id),
	beginR(0),
	endR(0),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

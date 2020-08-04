#include "cacheline.h"
#include "common.h"
#include "action.h"
#include "model.h"
#include "execution.h"


CacheLine::CacheLine(void *address) :
	id(getCacheID(address)),
	lastWrite(NULL),
	lastCacheOp(NULL),
	beginR(0),
	endR(0)
{
}

CacheLine::CacheLine(uintptr_t _id) :
	id(_id),
	lastWrite(NULL),
	lastCacheOp(NULL),
	beginR(0),
	endR(0)
{
}

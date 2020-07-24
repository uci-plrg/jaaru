#include "cacheline.h"
#include "common.h"
#include "action.h"
#include "model.h"
#include "execution.h"

#define RANGEUNDEFINED (modelclock_t)-1

CacheLine::CacheLine(void *address) :
	id(getCacheID(address)),
	beginR(RANGEUNDEFINED),
	endR(RANGEUNDEFINED),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

CacheLine::CacheLine(uintptr_t _id) :
	id(_id),
	beginR(RANGEUNDEFINED),
	endR(RANGEUNDEFINED),
	lastWrite(NULL),
	lastCacheOp(NULL)
{
}

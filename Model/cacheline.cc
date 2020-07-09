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
	lastCacheOp(NULL)
{
}

CacheLine::CacheLine(uintptr_t _id) :
	id(_id),
	beginR(RANGEUNDEFINED),
	endR(RANGEUNDEFINED),
	lastCacheOp(NULL)
{
}

modelclock_t CacheLine::getPersistentSeqNumber() const
{
	if(endR == RANGEUNDEFINED){
		return beginR;
	} else {
		return endR;
	}

}

void CacheLine::applyWrite(ModelAction * write){
    if(beginR == RANGEUNDEFINED && endR == RANGEUNDEFINED) {
		beginR = write->get_seq_number();
	} else if (beginR != RANGEUNDEFINED && endR != RANGEUNDEFINED) {
		// There's a write on a cache line that already persist.
		ASSERT(0);
	}
}

void CacheLine::persistCacheLine()
{
	ASSERT(lastCacheOp);
	ASSERT(endR == RANGEUNDEFINED);
	endR = lastCacheOp->get_seq_number();
}

void CacheLine::persistUntil(modelclock_t opclock)
{
    ASSERT(beginR != RANGEUNDEFINED);
    if(beginR < opclock){
        beginR = opclock;
    }
}
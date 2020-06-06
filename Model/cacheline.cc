#include <cmath>
#include "cacheline.h"
#include "common.h"
#include "action.h"

#define UNDEFINED (modelclock_t)-1

CacheLine::CacheLine(void *address) :
		beginR(UNDEFINED),
		endR(UNDEFINED),
		id(getCacheID(address))
{
}

CacheLine::CacheLine(uintptr_t _id) :
		beginR(UNDEFINED),
		endR(UNDEFINED),
		id(_id)
{
}

void CacheLine::applyWrite(ModelAction *write)
{
	ASSERT( ((uintptr_t)write->get_location() & 0x3) == 0);
	if(beginR == UNDEFINED && endR == UNDEFINED){
		beginR = write->get_seq_number();
	}
}

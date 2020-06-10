#include "varrange.h"
#include "action.h"

VarRange::VarRange(void *_address):
        beginR(RANGEUNDEFINED),
        endR(RANGEUNDEFINED),
        address(_address)
{
}

void VarRange::applyWrite(ModelAction * write){
    if(beginR == RANGEUNDEFINED && endR == RANGEUNDEFINED) {
		beginR = write->get_seq_number();
	} else if (beginR != RANGEUNDEFINED && endR != RANGEUNDEFINED) {
		beginR = endR;
		endR = RANGEUNDEFINED;
	}
}

void VarRange::persistUntil(modelclock_t opclock)
{
    ASSERT(beginR != RANGEUNDEFINED);
    if(beginR < opclock){
        beginR = opclock;
    }
}
#include "cacheline.h"
#include "common.h"
#include "action.h"
#include "varrange.h"
#include "model.h"
#include "execution.h"

CacheLine::CacheLine(void *address) :
	varSet(),
	id(getCacheID(address)),
	lastCacheOp(NULL)
{
}

void CacheLine::applyWrite(ModelAction *write)
{
	ASSERT( ((uintptr_t)write->get_location() & 0x3) == 0);
	VarRange tmp(write->get_location());
	VarRange* variable = varSet.get(&tmp);
	if(variable == NULL) {
		//First time writing to the variable.
		variable = new VarRange(write->get_location());
		varSet.add(variable);
	} 
	variable->applyWrite(write);
}

void CacheLine::persistCacheLine()
{
	ASSERT(lastCacheOp);
	VarRangeSetIter *iter = varSet.iterator();
	while(iter->hasNext()){
		VarRange *variable = iter->next();
		ModelAction* prevWrite = model->get_execution()->get_last_write_before_op(variable->getVarAddress(), lastCacheOp);
		ASSERT(prevWrite);
		variable->setEndRange(prevWrite->get_seq_number());
	}
	delete iter;
}


VarRange* CacheLine::getVariable(void *address) 
{
	VarRange tmp(address);
	return varSet.get(&tmp);
}

void CacheLine::persistUntil(modelclock_t opclock)
{
	VarRangeSetIter *iter = varSet.iterator();
	while(iter->hasNext()){
		VarRange *variable = iter->next();
		variable->persistUntil(opclock);
	}
	delete iter;
}
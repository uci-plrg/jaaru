
#include "classlist.h"
#include "cacheline.h"
#include "action.h"
#include "varrange.h"

unsigned int cacheLineHashFunction ( CacheLine *cl)
{
	if(cl == NULL)
		return 0;
	return (unsigned int) cl->getId();
}

bool cacheLineEquals(CacheLine *c1, CacheLine *c2)
{
	if (c1 == NULL && c2 == NULL)
		return true;
	if(c1 == NULL || c2 == NULL)
		return false;
	return c1->getId() == c2->getId();
}

unsigned int varRangeHashFunction ( VarRange *v1)
{
	if(v1 == NULL){
		return 0;
	}
	return static_cast<unsigned int> ((uintptr_t) v1->getVarAddress());
}

bool varRangeEquals(VarRange *v1, VarRange *v2)
{
	if( v1 == NULL && v2 == NULL){
		return true;
	} else if (v1 == NULL || v2 == NULL){
		return false;
	}
	return v1->getVarAddress() == v2->getVarAddress();
}
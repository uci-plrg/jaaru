
#include "classlist.h"
#include "cacheline.h"
#include "action.h"

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

unsigned int cacheLineIDHashFunction ( ModelAction* a1)
{
	if(a1 == NULL) {
		return 0;
	}
	return (unsigned int) getCacheID(a1->get_location() ) ;
}

bool cacheLineIDEquals(ModelAction *a1, ModelAction *a2)
{
	if( a1 == NULL && a2 == NULL) {
		return true;
	} else if( a1 == NULL || a2 == NULL ) {
		return false;
	}
	return getCacheID(a1->get_location()) == getCacheID(a2->get_location());
}
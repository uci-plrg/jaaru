
#include "classlist.h"
#include "cacheline.h"

unsigned int cacheLineHashFunction ( CacheLine *cl){
	if(cl == NULL)
		return 0;
	return (unsigned int) cl->getId();
}

bool cacheLineEquals(CacheLine *c1, CacheLine *c2){
	if (c1 == NULL && c2 == NULL)
		return true;
	if(c1 == NULL || c2 == NULL)
		return false;
	return c1->getId() == c2->getId();
}

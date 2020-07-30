#include "hashfunction.h"
#include "classlist.h"
#include "cacheline.h"
#include "action.h"


/**
 * Hash function for 64-bit integers
 * https://gist.github.com/badboy/6267743#64-bit-to-32-bit-hash-functions
 */
unsigned int int64_hash(uint64_t key) {
	key = (~key) + (key << 18);	// key = (key << 18) - key - 1;
	key = key ^ (key >> 31);
	key = key * 21;	// key = (key + (key << 2)) + (key << 4);
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	return (unsigned int) key;
}

unsigned int cacheLineHashFunction ( CacheLine *cl) {
	return ((unsigned int) (cl->getId() >> 6));
}

bool cacheLineEquals(CacheLine *c1, CacheLine *c2) {
	return c1->getId() == c2->getId();
}

unsigned int WriteVecHashFunction(SnapVector<Pair<ModelExecution *, ModelAction *> > * vec) {
	unsigned int hash = vec->size();
	for(uint i = 0;i<vec->size();i++) {
		Pair<ModelExecution *, ModelAction *> pair = (*vec)[i];
		hash ^= (uint)(((uintptr_t)pair.p1>>3) ^ ((uintptr_t)pair.p2>>3));
		hash = hash >> 3 ^ hash << 10;
	}
	return hash;
}

bool WriteVecEquals(SnapVector<Pair<ModelExecution *, ModelAction *> > * vec1,
										SnapVector<Pair<ModelExecution *, ModelAction *> > * vec2) {
	if (vec1->size() != vec2->size())
		return false;

	for(uint i = 0;i<vec1->size();i++) {
		Pair<ModelExecution *, ModelAction *> pair1 = (*vec1)[i];
		Pair<ModelExecution *, ModelAction *> pair2 = (*vec2)[i];
		if (pair1.p1 != pair2.p1 || pair1.p2 != pair2.p2)
			return false;
	}
	return true;
}

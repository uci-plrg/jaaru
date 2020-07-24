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


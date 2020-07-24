/**
 * test.h: A header file that contains the info needed by all the test cases
 *
 **/

#ifndef TEST_H
#define TEST_H
#include <atomic>
using namespace std;
#define TEST_CACHE_LINE_SIZE 64
memory_order orders[7] = {
	memory_order_relaxed, memory_order_consume, memory_order_acquire,
	memory_order_release, memory_order_acq_rel, memory_order_seq_cst,
};

enum CacheOp {
	CLFLUSH,
	CLFLUSHOPT,
	CLWB
};

inline void mfence() {
	asm volatile ("mfence" ::: "memory");
}

inline void cacheOperation(CacheOp cop, char *data, unsigned int len)
{
	volatile char *ptr = (char *)((unsigned long)data & ~(TEST_CACHE_LINE_SIZE - 1));
	for (;ptr < data+len;ptr += TEST_CACHE_LINE_SIZE) {
		switch(cop) {
		case CLFLUSH:
			asm volatile ("clflush %0" : "+m" (*(volatile char *)ptr));
			break;
		case CLFLUSHOPT:
			asm volatile (".byte 0x66; clflush %0" : "+m" (*(volatile char *)(ptr)));
			break;
		case CLWB:
			asm volatile (".byte 0x66; xsaveopt %0" : "+m" (*(volatile char *)(ptr)));
			break;
		default:
			printf("Fatal Error: Unknown cache operation is used\n");
			exit(-1);
		}
	}
}


#endif

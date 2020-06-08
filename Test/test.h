/**
 * test.h: A header file that contains the info needed by all the test cases
 *
 **/

#ifndef TEST_H
#define TEST_H
#include <atomic>
using namespace std;

memory_order orders[7] = {
	memory_order_relaxed, memory_order_consume, memory_order_acquire,
	memory_order_release, memory_order_acq_rel, memory_order_seq_cst,
};


#endif

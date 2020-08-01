#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"
#include "pminterface.h"

using namespace std;

atomic<unsigned int> x(0);
int array[2000];
atomic<unsigned int> y(0);

extern "C" {
__attribute__ ((visibility ("default"))) void restart();
}

void func1() {
	x.store(1);
	x.store(5);
	cacheOperation(CLFLUSHOPT, (char *)&x, sizeof(unsigned int));
	mfence();
	y.store(1);
	cacheOperation(CLFLUSHOPT, (char *)&x, sizeof(unsigned int));
	mfence();
}


void func2() {
	int a = y.load();
	if (a) {
		int b = x.load();
		printf("%d", b);
	}
}

int main() {
	if (getRegionFromID(0) == NULL) {
		setRegionFromID(0, (void *) 1);
		func1();
	} else
		func2();

	return EXIT_SUCCESS;
}

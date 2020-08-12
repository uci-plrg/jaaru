#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"
#include "pminterface.h"

using namespace std;

struct foo {
	atomic<unsigned int> x;
	int array[2000];
	atomic<unsigned int> y;
};

struct foo * ptr = NULL;

extern "C" {
__attribute__ ((visibility ("default"))) void restart();
}

void func1() {
	ptr->x.store(1);
	ptr->x.store(5);
	cacheOperation(CLFLUSHOPT, (char *)&ptr->x, sizeof(unsigned int));
	mfence();
	ptr->y.store(1);
	cacheOperation(CLFLUSHOPT, (char *)&ptr->x, sizeof(unsigned int));
	mfence();
}


void func2() {
	int a = ptr->y.load();
	if (a) {
		int b = ptr->x.load();
		printf("%d", b);
	}
}

int main() {
	if (getRegionFromID(0) == NULL) {
		setRegionFromID(0, ptr = (struct foo *) calloc(sizeof(struct foo), 1));
		func1();
	} else {
		ptr = (struct foo *) getRegionFromID(0);
		func2();
	}
	return EXIT_SUCCESS;
}

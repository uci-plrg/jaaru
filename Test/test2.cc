#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"

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

__attribute__ ((visibility ("default"))) void restart() {
	func2();
}

int main() {
	if (ptr == NULL)
		ptr = (struct foo *) calloc(sizeof(struct foo),1);
	func1();
	return EXIT_SUCCESS;
}

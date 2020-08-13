#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include <assert.h>
#include "test.h"
#include "libpmem.h"

using namespace std;
typedef struct Node {
	uint64_t key;
	char value[8];
} Node;

int main() {
	Node* n1 = (Node*)pmem_map_file("/path/to/Node", sizeof(Node), 0, 0, NULL, NULL);
	n1->key = 12;
	const char * message = "Hello!";
	pmem_memcpy_persist(n1->value, message, 8);
	assert( pmem_is_pmem(n1, sizeof(Node)) );
	pmem_persist(n1, sizeof(Node));
	Node *n2 = (Node*)pmem_map_file("/path/to/Node", sizeof(Node), 0, 0, NULL, NULL);
	assert(n1 == n2);
	printf("N1: Key = %lu\tValue = %s\n", n1->key, n1->value);
	printf("N2: Key = %lu\tValue = %s\n", n2->key, n2->value);
	Node* newNode = (Node*)pmem_map_file("/path/to/Node2", sizeof(Node), 0, 0, NULL, NULL);
	n1->key += 1000;
	message = "Bye!";
	pmem_memcpy_persist(newNode, n1, sizeof(Node));
	printf("New Node: Key = %lu\tValue = %s\n", n2->key, n2->value);
	return EXIT_SUCCESS;
}


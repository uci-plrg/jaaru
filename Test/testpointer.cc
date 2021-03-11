#include <stdlib.h>
#include <cstdio>
#include <malloc.h>
#include <assert.h>
#define CACHE_LINE_SIZE 64
typedef struct Node
{
	int value;
	volatile Node *next;
} Node;

int main()
{
	volatile Node* b = (Node *) memalign(CACHE_LINE_SIZE, sizeof(Node));
	b->value = 22;
	b->next = NULL;
	volatile Node* a = (Node *) memalign(CACHE_LINE_SIZE, sizeof(Node));
	a->value = 11;
	a->next = b;
	volatile Node * bcopy = a->next;
}
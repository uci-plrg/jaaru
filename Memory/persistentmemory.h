#ifndef PERSISTENTMEMORY_H
#define PERSISTENTMEMORY_H
#include "classlist.h"

void initializePersistentMemory();
bool isPersistent(void *address, uint size);

extern void * persistentMemoryRegion;

#define PERSISTENT_MEMORY_DEFAULT  (((size_t)1 << 20) * 100)	//100mb for program
#endif

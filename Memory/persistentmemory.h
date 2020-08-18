#ifndef PMEM_H
#define PMEM_H
#include "classlist.h"

void pmem_init();

extern void * persistentMemoryRegion;
extern FileMap *fileIDMap;
extern mspace mallocSpace;
void init_memory_ops();
#define PERSISTENT_MEMORY_DEFAULT  (((size_t)1 << 20) * 100)	//100mb for program
#endif

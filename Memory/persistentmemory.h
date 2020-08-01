#ifndef PERSISTENTMEMORY_H
#define PERSISTENTMEMORY_H
#include "classlist.h"

void initializePersistentMemory();
bool isPersistent(void *address, uint size);

#endif

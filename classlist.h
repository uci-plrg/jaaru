#ifndef CLASSLIST_H
#define CLASSLIST_H
#include <inttypes.h>
#include "stl-model.h"
#include "hashset.h"

typedef int thread_id_t;

#define THREAD_ID_T_NONE        -1

typedef unsigned int modelclock_t;

class CacheLine;
class ThreadMemory;
class Thread;
class ModelAction;
class ModelExecution;
class Scheduler;
class ClockVector;
class Fuzzer;
class ModelChecker;

typedef SnapList<ModelAction *> action_list_t;

unsigned int cacheLineHashFunction ( CacheLine * cl);
bool cacheLineEquals(CacheLine *c1, CacheLine *c2);

typedef HashSet<CacheLine *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineHashFunction, cacheLineEquals> CacheLineSet;
typedef HSIterator<CacheLine *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineHashFunction, cacheLineEquals> CacheLineSetIter;
typedef HashSet<uint64_t, uint64_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free> ValueSet;
typedef HSIterator<uint64_t, uint64_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free> ValueSetIter;

extern volatile int modellock;
#endif

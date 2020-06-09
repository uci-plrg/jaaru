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
class actionlist;

typedef SnapList<ModelAction *> simple_action_list_t;
typedef actionlist action_list_t;

unsigned int cacheLineHashFunction ( CacheLine * cl);
bool cacheLineEquals(CacheLine *c1, CacheLine *c2);
unsigned int cacheLineIDHashFunction ( ModelAction* a1);
bool cacheLineIDEquals(ModelAction *a1, ModelAction *a2);



typedef HashSet<CacheLine *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineHashFunction, cacheLineEquals> CacheLineSet;
typedef HSIterator<CacheLine *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineHashFunction, cacheLineEquals> CacheLineSetIter;
typedef HashSet<ModelAction *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineIDHashFunction, cacheLineIDEquals> MemoryBufferSet;
typedef HSIterator<ModelAction *, uintptr_t, 0, snapshot_malloc, snapshot_calloc, snapshot_free, cacheLineIDHashFunction, cacheLineIDEquals> MemoryBufferSetIter;

extern volatile int modellock;
#endif

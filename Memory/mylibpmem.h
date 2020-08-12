#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <mymemory.h>
#include <atomicapi.h>
#include <map.h>
#include <stdio.h>
#include <string.h>
#include <snapshot.h>

extern "C" {

static mspace pmemBase = NULL;
struct StrHashTable pathToAddr;

void
model_pmem_drain(void);

void
model_pmem_flush(const void *addr, size_t len);
void
model_pmem_flush_deep(const void *addr, size_t len);
//The semantics of pmem_deep_flush() function is the same as pmem_flush() function except that pmem_deep_flush() is indifferent to PMEM_NO_FLUSH environment variable 

void
model_pmem_persist(const void *addr, size_t len);

void
model_pmem_msync(const void *addr, size_t len);
//since we're not calling msync, we don't need to round to pagesize

int
model_pmem_is_pmem(const void *addr, size_t len);

void 
model_pmem_map_file_init();

void *
model_pmem_map_file(const char *path, size_t len);

//not in pmem.c: just for our file mapping
void*
model_pmem_file(const char *path);

void
pmem_unmap(void *addr, size_t len);



//ask about:

//pmem_is_pmem, specifically, if we can tell if a memory range is pmem or not
//pmem_file, specifically if we have mapping pmem
//cpy, move,

//ask about implementation
void * model_pmem_memmove(void *pmemdest, const void *src, size_t len);
void * model_pmem_memcpy(void *pmemdest, const void *src, size_t len);
void * model_pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len);
void * model_pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len);
void * model_pmem_memset_nodrain(void *pmemdest, int c, size_t len, unsigned flags);
}



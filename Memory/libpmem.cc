#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include "mymemory.h"
#include "atomicapi.h"
#include "snapshot.h"
#include "libpmem.h"
#include "mymemory.h"
#include "pmcheckapi.h"
#include "persistentmemory.h"
#include "pminterface.h"
#include "model.h"

#define CACHE_LINE_SIZE 64
FileMap *fileIDMap = NULL;
mspace mallocSpace = NULL;


void pmem_init() {
	mallocSpace = create_mspace_with_base(persistentMemoryRegion, PERSISTENT_MEMORY_DEFAULT, 1);
	if(fileIDMap == NULL) {
		fileIDMap = new FileMap();
	} else {
		fileIDMap->reset();
	}
}

void createFileIDMap(){
	if(fileIDMap == NULL) {
		fileIDMap = new FileMap();
	}
}

void * pmdk_malloc(size_t size) {
	if(!mallocSpace) {
		pmem_init();
	}
	void * addr = mspace_malloc(mallocSpace, size);
	ASSERT(addr);
	return addr;
}

void pmem_drain(void)
{
	pmc_sfence();
}

void pmem_flush(const void *addr, size_t len)
{
	char *ptr = (char *)((unsigned long)addr &~(CACHE_LINE_SIZE-1));
	for(;ptr<(char*)addr + len;ptr += CACHE_LINE_SIZE) {
#ifdef CLFLUSH
		pmc_clflush(ptr);
#elif CLFLUSH_OPT
		pmc_clflushopt(ptr);
#elif CLWB
		pmc_clwb(ptr);
#endif
	}
}

int pmem_has_auto_flush(void) {
	//TODO: Adding support for auto flushing
	return false;
}


void pmem_flush_deep(const void *addr, size_t len)
{
	pmem_flush(addr, len);
}

void pmem_persist(const void *addr, size_t len)
{
	pmem_flush(addr, len);
	pmem_drain();
}

/**
 * Calls pmem_persist() if the range is persistent otherwise nothing is being done.
 */
int pmem_msync(const void *addr, size_t len)
{
	if(pmem_is_pmem(addr, len)) {
		pmem_persist(addr, len);
	}
	return true;
}

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp)
{
	createModelIfNotExist();
	uint64_t id = fileIDMap->get(path);
	if( id!= 0) {	//No one is using ID = 0
		return getRegionFromID(id);
	}
	id = getNextRegionID();
	size_t pathSize = strlen(path);
	char * pathCopy = (char*) pmdk_malloc(sizeof(char)*pathSize);
	memmove(pathCopy, path, sizeof(char)*pathSize);
	fileIDMap->put(pathCopy, id);
	void * addr = pmdk_malloc(len);
	setRegionFromID(id, addr);
	if(is_pmemp)
		*is_pmemp = true;
	return addr;
}

int pmem_unmap(void *ptr, size_t len)
{
	if (mallocSpace) {
		if ((((uintptr_t)ptr) >= ((uintptr_t)persistentMemoryRegion)) &&
				(((uintptr_t)ptr) < (((uintptr_t)persistentMemoryRegion) + PERSISTENT_MEMORY_DEFAULT))) {
			mspace_free(mallocSpace, ptr);
			return 0;
		}
	}
	return -1;
}

void pmem_deep_flush(const void *addr, size_t len) {
	pmem_flush(addr, len);
}

int pmem_deep_drain(const void *addr, size_t len) {
	pmem_drain();
	return true;
}

int pmem_deep_persist(const void *addr, size_t len) {
	pmem_deep_flush(addr, len);
	return pmem_deep_drain(addr, len);
}

int pmem_has_hw_drain(void) {
	return false;
}

/**
 * Calling move operation with CLFLUSH and fence
 */
void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len) {
	return pmem_memmove(pmemdest, src, len, (unsigned)(~PMEM_F_MEM_NODRAIN & ~PMEM_F_MEM_NOFLUSH));
}

/**
 * Calling move operation with CLFLUSH and fence
 */
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len) {
	return pmem_memcpy(pmemdest, src, len, (unsigned)(~PMEM_F_MEM_NODRAIN & ~PMEM_F_MEM_NOFLUSH));
}

/**
 * Calling move operation with CLFLUSH and fence
 */
void *pmem_memset_persist(void *pmemdest, int c, size_t len) {
	return pmem_memset(pmemdest, c, len,(unsigned)(~PMEM_F_MEM_NODRAIN & ~PMEM_F_MEM_NOFLUSH));
}

/**
 * Calling move operation with CLFLUSH but without fence
 */
void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len) {
	return pmem_memmove(pmemdest, src, len, PMEM_F_MEM_NODRAIN);
}

/**
 * Calling memcpy operation with CLFLUSH but without fence
 */
void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len) {
	return pmem_memcpy(pmemdest, src, len, PMEM_F_MEM_NODRAIN);
}

/**
 * Calling memcpy operation with CLFLUSH but without fence
 */
void *pmem_memset_nodrain(void *pmemdest, int c, size_t len) {
	return pmem_memset(pmemdest, c, len, PMEM_F_MEM_NODRAIN);
}


const char * memmovestring = "memmove";
void *pmem_memmove(void *dst, const void *src, size_t n, unsigned flags) {
	for(uint i=0;i<n;) {
		if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint64_t val = pmc_load64(((char *) src) + i, memmovestring);
			pmc_store64(((char *) dst)+i, val, memmovestring);
			i+=8;
		} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint32_t val = pmc_load32(((char *) src) + i, memmovestring);
			pmc_store32(((char *) dst)+i, val, memmovestring);
			i+=4;
		} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t val = pmc_load16(((char *) src) + i, memmovestring);
			pmc_store16(((char *) dst)+i, val, memmovestring);
			i+=2;
		} else {
			uint8_t val = pmc_load8(((char *) src) + i, memmovestring);
			pmc_store8(((char *) dst)+i, val, memmovestring);
			i=i+1;
		}
	}

	if ((flags & PMEM_F_MEM_NOFLUSH) == 0) {
		pmem_flush(dst, n);
	}

	if ((flags & (PMEM_F_MEM_NODRAIN | PMEM_F_MEM_NOFLUSH)) == 0) {
		pmem_drain();
	}
	return dst;
}

const char * memstring = "memcpy";
void *pmem_memcpy(void *dst, const void *src, size_t n, unsigned flags) {
	for(uint i=0;i<n;) {
		if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint64_t val = pmc_load64(((char *) src) + i, memstring);
			pmc_store64(((char *) dst)+i, val, memstring);
			i+=8;
		} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint32_t val = pmc_load32(((char *) src) + i, memstring);
			pmc_store32(((char *) dst)+i, val, memstring);
			i+=4;
		} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t val = pmc_load16(((char *) src) + i, memstring);
			pmc_store16(((char *) dst)+i, val, memstring);
			i+=2;
		} else {
			uint8_t val = pmc_load8(((char *) src) + i, memstring);
			pmc_store8(((char *) dst)+i, val, memstring);
			i=i+1;
		}
	}

	if ((flags & (PMEM_F_MEM_NOFLUSH)) == 0) {
		pmem_flush(dst, n);
	}

	if ((flags & (PMEM_F_MEM_NODRAIN | PMEM_F_MEM_NOFLUSH)) == 0) {
		pmem_drain();
	}
	return dst;
}

const char * memsetstring = "memset";
void *pmem_memset(void *dst, int c, size_t n, unsigned flags) {
	uint8_t cs = c&0xff;
	for(uint i=0;i<n;) {
		if ((((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			uint64_t cs3 = cs2 << 16 | cs2;
			uint64_t cs4 = cs3 << 32 | cs3;
			pmc_store64(((char *) dst)+i, cs4, memsetstring);
			i+=8;
		} else if ((((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			uint32_t cs3 = cs2 << 16 | cs2;
			pmc_store32(((char *) dst)+i, cs3, memsetstring);
			i+=4;
		} else if ((((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			pmc_store16(((char *) dst)+i, cs2, memsetstring);
			i+=2;
		} else {
			pmc_store8(((char *) dst)+i, cs, memsetstring);
			i=i+1;
		}
	}
	if ((flags & (PMEM_F_MEM_NOFLUSH)) == 0) {
		pmem_flush(dst, n);
	}

	if ((flags & (PMEM_F_MEM_NODRAIN | PMEM_F_MEM_NOFLUSH)) == 0) {
		pmem_drain();
	}
	return dst;
}


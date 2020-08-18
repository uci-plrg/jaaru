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
#define	POOL_HDR_UUID_LEN	16 /* uuid byte length */
typedef unsigned char uuid_t[POOL_HDR_UUID_LEN]; /* 16 byte binary uuid value */
struct pool_set_part {
	/* populated by a pool set file parser */
	const char *path;
	size_t filesize;	/* aligned to page size */
	// int fd;
	int created;		/* indicates newly created (zeroed) file */

	/* util_poolset_open/create */
	void *hdr;		/* base address of header */
	size_t hdrsize;		/* size of the header mapping */
	void *addr;		/* base address of the mapping */
	size_t size;		/* size of the mapping - page aligned */
	int rdonly;
	uuid_t uuid;
};

struct pool_replica {
	unsigned nparts;
	size_t repsize;		/* total size of all the parts (mappings) */
	int is_pmem;		/* true if all the parts are in PMEM */
	struct pool_set_part part[];
};

struct pool_set {
	unsigned nreplicas;
	uuid_t uuid;
	int rdonly;
	int zeroed;		/* true if all the parts are new files */
	size_t poolsize;	/* the smallest replica size */
	struct pool_replica *replica[];
};

extern int util_header_create(struct pool_set *set, unsigned repidx, unsigned partidx,
	const char *sig, uint32_t major, uint32_t compat, uint32_t incompat,
	uint32_t ro_compat);

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


void pmem_init() {
	mallocSpace = create_mspace_with_base(persistentMemoryRegion, PERSISTENT_MEMORY_DEFAULT, 1);
	if(fileIDMap == NULL) {
		fileIDMap = new FileMap();
	} else {
		fileIDMap->reset();
	}
}

static void pmem_register_file(const char *path, void * addr) {
	createModelIfNotExist();
	uint64_t id = fileIDMap->get(path);
	ASSERT( id == 0);
	id = getNextRegionID();
	size_t pathSize = strlen(path) + 1;
	char * pathCopy = (char*) pmdk_malloc(sizeof(char)*pathSize);
	memmove(pathCopy, path, sizeof(char)*pathSize);
	fileIDMap->put(pathCopy, id);
	setRegionFromID(id, addr);
	// Create a dummpy file so no change is required in the client
	FILE *fp = fopen(path, "w+");
	fclose(fp);
}

int util_poolset_create(struct pool_set **setp, const char *path, size_t poolsize, size_t minsize, int pagesize)
{
	struct pool_set *set = (struct pool_set *)pmdk_malloc(sizeof (struct pool_set) + sizeof (struct pool_replica *));
	struct pool_replica *rep= (struct pool_replica *) pmdk_malloc(sizeof (struct pool_replica) + sizeof (struct pool_set_part));
	set->replica[0] = rep;

	rep->part[0].filesize = poolsize;
	size_t pathSize = strlen(path) + 1;
	char * pathCopy = (char*) pmdk_malloc(sizeof(char)*pathSize);
	memmove(pathCopy, path, sizeof(char)*pathSize);
	rep->part[0].path = pathCopy;
	// rep->part[0].fd = fd;
	rep->part[0].created = 1;
	rep->part[0].hdr = NULL;
	rep->part[0].addr = NULL;
	rep->nparts = 1;
	/* round down to the nearest page boundary */
	rep->repsize = rep->part[0].filesize & ~(pagesize - 1);
	set->poolsize = rep->repsize;
	set->nreplicas = 1;
	//REGISTER PATH...
	pmem_register_file(path, set);
	*setp = set;
	return 0;
}

int util_replica_create(struct pool_set *set, unsigned repidx, int flags,
	const char *sig, uint32_t major, uint32_t compat, uint32_t incompat, uint32_t ro_compat, int Pool_hdr_size, int pagesize)
{
	struct pool_replica *rep = set->replica[repidx];
	{
		struct pool_set_part *part = &rep->part[0];
		part->addr = pmdk_malloc(rep->repsize);
		part->size = rep->repsize;
		//Header allocation
		for (unsigned p = 0; p < rep->nparts; p++){
			struct pool_set_part * part = &rep->part[p];
			part->hdrsize = Pool_hdr_size;
			part->hdr = pmdk_malloc(sizeof(Pool_hdr_size));
		}
	}
	/* create headers, set UUID's */
	for (unsigned p = 0; p < rep->nparts; p++) {
		if (util_header_create(set, repidx, p, sig, major,
				compat, incompat, ro_compat) != 0) {
			return -1;
		}
	}
	set->zeroed &= rep->part[0].created;
	{
		size_t mapsize = rep->part[0].filesize & ~(pagesize - 1);
		void *addr = (char *)rep->part[0].addr + mapsize;
		for (unsigned p = 1; p < rep->nparts; p++) {
			struct pool_set_part *part = &rep->part[p];
			size_t size = (part->filesize & ~(pagesize - 1)) - Pool_hdr_size;
			part->addr = addr;
			part->size = size;
			mapsize += rep->part[p].size;
			set->zeroed &= rep->part[p].created;
			addr = (char *)addr + rep->part[p].size;
		}
		rep->is_pmem = pmem_is_pmem(rep->part[0].addr, rep->part[0].size);
		ASSERT(mapsize == rep->repsize);
	}
	return 0;
}


int util_poolset_open(struct pool_set **setp, const char *path, size_t minsize) {
	*setp = (struct pool_set *)pmem_map_file(path, minsize, 0, 0, NULL, NULL);
	return 0;
}

// int util_map_hdr(struct pool_set_part *part, int flags, int POOL_HDR_SIZE)
// {
// 	char * headerpath = (char*) calloc( strlen(part->path) + 10 , sizeof(char*));
// 	strcat(headerpath, part->path);
// 	strcat(headerpath, "-header");
// 	model_print("Log: header path: %s", headerpath);
// 	void *hdrp = pmem_map_file(headerpath, POOL_HDR_SIZE, flags, 0, 0, NULL);
// 	part->hdrsize = POOL_HDR_SIZE;
// 	part->hdr = hdrp;
// 	free(headerpath);
// 	return 0;
// }


// int util_map_part(struct pool_set_part *part, void *addr, size_t size, size_t offset, int flags, unsigned long Pagesize)
// {
// 	model_print("Log: part path: %s", part->path);
// 	ASSERT((uintptr_t)addr % Pagesize == 0);
// 	ASSERT(offset % Pagesize == 0);
// 	ASSERT(size % Pagesize == 0);
// 	ASSERT(((off_t)offset) >= 0);
// 	void *addrp = pmem_map_file(part->path, size, flags, 0, 0, NULL);
// 	ASSERT( ((char*)addrp + offset) != addr);
// 	if(!size) { //Address already allocated for the entire replica	
// 		size = (part->filesize & ~(Pagesize - 1)) - offset;
// 		addrp = addr;
// 	}
// 	addrp = (char *)addrp + offset;
// 	part->addr = addrp;
// 	part->size = size;
// 	if(!pmem_is_pmem(addrp, size)) {
// 		return -1;
// 	}
// 	return 0;
// }


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
	return 0;
}

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp)
{
	createModelIfNotExist();
	uint64_t id = fileIDMap->get(path);
	if( id!= 0) {	//No one is using ID = 0
		return getRegionFromID(id);
	}
	id = getNextRegionID();
	size_t pathSize = strlen(path) + 1;
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


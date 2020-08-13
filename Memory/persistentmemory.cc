#include "persistentmemory.h"
#include "config.h"
#include "pmcheckapi.h"
#include <string.h>
#include <dlfcn.h>
#include "atomicapi.h"
#include "model.h"
#include "libpmem.h"

extern "C" {

};

int FILLBYTE=0;
void * persistentMemoryRegion;

int pmem_is_pmem(const void *address, size_t size) {
	return ((persistentMemoryRegion != NULL) &&
					(((uintptr_t)address) >= ((uintptr_t)persistentMemoryRegion)) &&
					(((uintptr_t)address) < (((uintptr_t)persistentMemoryRegion) + PERSISTENT_MEMORY_DEFAULT)));
}


void * (*memcpy_real)(void * dst, const void *src, size_t n) = NULL;
void * memcpy(void * dst, const void * src, size_t n) {
	if (pmem_is_pmem(dst,1) && !inside_model) {
		return pmem_memcpy(dst, src, n, PMEM_F_MEM_NOFLUSH);
	} else {
		if (!memcpy_real) {
			memcpy_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memcpy");
		}
		return memcpy_real(dst, src, n);
	}
}

void * (*memmove_real)(void * dst, const void *src, size_t len) = NULL;
void * memmove(void *dst, const void *src, size_t n) {
	if (pmem_is_pmem(dst,1) && !inside_model) {
		return pmem_memmove(dst, src, n, PMEM_F_MEM_NOFLUSH);
	} else {
		if (!memmove_real) {
			memmove_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memmove");
		}
		return memmove_real(dst, src, n);
	}
}

void * (*volatile memset_real)(void * dst, int c, size_t len) = NULL;

void * realmemset(void *dst, int c, size_t n) {
	if (!memset_real) {
		memset_real = (void * (*)(void * dst, int c, size_t n))dlsym(RTLD_NEXT, "memset");
	}
	return memset_real(dst, c, n);
}

void * memset(void *dst, int c, size_t n) {
	if (pmem_is_pmem(dst,1) && !inside_model) {
		return pmem_memset(dst, c, n, PMEM_F_MEM_NOFLUSH);
	} else {
		if (((uintptr_t)memset_real) < 2) {
			if (memset_real == NULL) {
				memset_real = (void * (*)(void * dst, int c, size_t n)) 1;
				memset_real = (void * (*)(void * dst, int c, size_t n))dlsym(RTLD_NEXT, "memset");
			} else {
				//stuck in dynamic linker alloc cycle...
				for(size_t s=0;s<n;s++) {
					((volatile char *)dst)[s] = (char) c;
				}
				return dst;
			}
		}
		return memset_real(dst, c, n);
	}
}

void (*bzero_real)(void * dst, size_t len) = NULL;
const char * bzerostring = "bzero";
void bzero(void *dst, size_t n) {
	if (pmem_is_pmem(dst,1) && !inside_model) {
		for(uint i=0;i<n;) {
			if ((((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
				pmc_store64(((char *) dst)+i, 0, bzerostring);
				i+=8;
			} else if ((((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
				pmc_store32(((char *) dst)+i, 0, bzerostring);
				i+=4;
			} else if (((((uintptr_t)dst+i))&1)==0 && (i + 2) <= n) {
				pmc_store16(((char *) dst)+i, 0, bzerostring);
				i+=2;
			} else {
				pmc_store8(((char *) dst)+i, 0, bzerostring);
				i=i+1;
			}
		}

	} else {
		if (!bzero_real) {
			bzero_real = (void (*)(void * dst, size_t len))dlsym(RTLD_NEXT, "bzero");
		}
		bzero_real(dst, n);
	}
}

char * (*strcpy_real)(char * dst, const char *src) = NULL;
const char * strcpystring = "strcpy";
char * strcpy(char *dst, const char *src) {
	if (pmem_is_pmem(dst,1) && !inside_model) {
		size_t n = 0;
		while(src[n] != '\0') n++;
		for(uint i=0;i<n;) {
			if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
				uint64_t val = pmc_load64(((char *) src) + i, strcpystring);
				pmc_store64(((char *) dst)+i, val, strcpystring);
				i+=8;
			} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
				uint32_t val = pmc_load32(((char *) src) + i, strcpystring);
				pmc_store32(((char *) dst)+i, val, strcpystring);
				i+=4;
			} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
				uint16_t val = pmc_load16(((char *) src) + i, strcpystring);
				pmc_store16(((char *) dst)+i, val, strcpystring);
				i+=2;
			} else {
				uint8_t val = pmc_load8(((char *) src) + i, strcpystring);
				pmc_store8(((char *) dst)+i, val, strcpystring);
				i=i+1;
			}
		}
		return dst;
	} else {
		if (!strcpy_real) {
			strcpy_real = (char * (*)(char * dst, const char *src))dlsym(RTLD_NEXT, strcpystring);
		}
		return strcpy_real(dst, src);
	}
}

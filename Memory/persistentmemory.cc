#include "persistentmemory.h"
#include "malloc.h"
#include "config.h"
#include "pmcheckapi.h"
#include <string.h>
#include <dlfcn.h>
#include "model.h"

extern "C" {
void* __libc_malloc(size_t size);
void* __libc_calloc(size_t n, size_t size);
void* __libc_realloc(void* address, size_t size);
void* __libc_memalign(size_t alignment, size_t size);
void __libc_free(void* ptr);

typedef void * mspace;
extern void * mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);
extern void * mspace_realloc(mspace msp, void* mem, size_t newsize);
extern void * mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);
extern void * mspace_memalign(mspace msp, size_t alignment, size_t bytes);
extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
extern mspace create_mspace(size_t capacity, int locked);
};

void * persistentMemoryRegion;
mspace mallocSpace = NULL;

void initializePersistentMemory() {
	mallocSpace = create_mspace_with_base(persistentMemoryRegion, PERSISTENT_MEMORY_DEFAULT, 1);
}

bool isPersistent(void *address, uint size) {
	return ((persistentMemoryRegion != NULL) &&
					(((uintptr_t)address) >= ((uintptr_t)persistentMemoryRegion)) &&
					(((uintptr_t)address) < (((uintptr_t)persistentMemoryRegion) + PERSISTENT_MEMORY_DEFAULT)));
}

void *malloc(size_t size) {
	createModelIfNotExist();
	if (mallocSpace) {
		void * tmp = mspace_malloc(mallocSpace, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_malloc(size);
	}
}

void *calloc(size_t count, size_t size) {
	createModelIfNotExist();
	if (mallocSpace) {
		void * tmp = mspace_calloc(mallocSpace, count, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_calloc(count, size);
	}
}

void *realloc(void * ptr, size_t size) {
	createModelIfNotExist();
	if (mallocSpace) {
		void * tmp = mspace_realloc(mallocSpace, ptr, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_realloc(ptr, size);
	}
}

void *memalign(size_t align, size_t size) {
	createModelIfNotExist();
	if (mallocSpace) {
		void * tmp = mspace_memalign(mallocSpace, align, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_memalign(align, size);
	}
}

int posix_memalign(void ** addr, size_t align, size_t size) {
	createModelIfNotExist();
	void *tmp;
	if (mallocSpace) {
		tmp = mspace_memalign(mallocSpace, align, size);
	} else {
		tmp = __libc_memalign(align, size);
	}
	ASSERT(tmp);
	*addr = tmp;
	return 0;
}

void free(void *ptr) {
	createModelIfNotExist();
	if (mallocSpace) {
		if ((((uintptr_t)ptr) >= ((uintptr_t)persistentMemoryRegion)) &&
				(((uintptr_t)ptr) < (((uintptr_t)persistentMemoryRegion) + PERSISTENT_MEMORY_DEFAULT))) {
			mspace_free(mallocSpace, ptr);
			return;
		}
	}
	__libc_free(ptr);
	return;
}

void * (*memcpy_real)(void * dst, const void *src, size_t n) = NULL;
const char * memstring = "memcpy";
void * memcpy(void * dst, const void * src, size_t n) {
	if (isPersistent(dst,1) && !inside_model) {
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
		return dst;
	} else {
		if (!memcpy_real) {
			memcpy_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memcpy");
		}
		return memcpy_real(dst, src, n);
	}
}

void * (*memmove_real)(void * dst, const void *src, size_t len) = NULL;
const char * memmovestring = "memmove";
void * memmove(void *dst, const void *src, size_t n) {
	if (isPersistent(dst,1) && !inside_model) {
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
		return dst;
	} else {
		if (!memmove_real) {
			memmove_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memmove");
		}
		return memmove_real(dst, src, n);
	}
}

void * (*memset_real)(void * dst, int c, size_t len) = NULL;
const char * memsetstring = "memset";
void * memset(void *dst, int c, size_t n) {
	if (isPersistent(dst,1) && !inside_model) {
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
		return dst;
	} else {
		if (!memset_real) {
			memset_real = (void * (*)(void * dst, int c, size_t n))dlsym(RTLD_NEXT, "memset");
		}
		return memset_real(dst, c, n);
	}
}

void (*bzero_real)(void * dst, size_t len) = NULL;
const char * bzerostring = "bzero";
void bzero(void *dst, size_t n) {
	if (isPersistent(dst,1) && !inside_model) {
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
	if (isPersistent(dst,1) && !inside_model) {
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

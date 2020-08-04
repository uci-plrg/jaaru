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
	if (mallocSpace) {
		void * tmp = mspace_malloc(mallocSpace, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_malloc(size);
	}
}

void *calloc(size_t count, size_t size) {
	if (mallocSpace) {
		void * tmp = mspace_calloc(mallocSpace, count, size);
		ASSERT(tmp);
		//make sure we see the zero's...
		bzero(tmp, count * size);
		return tmp;
	} else {
		return __libc_calloc(count, size);
	}
}

void *realloc(void * ptr, size_t size) {
	if (mallocSpace) {
		void * tmp = mspace_realloc(mallocSpace, ptr, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_realloc(ptr, size);
	}
}

void *memalign(size_t align, size_t size) {
	if (mallocSpace) {
		void * tmp = mspace_memalign(mallocSpace, align, size);
		ASSERT(tmp);
		return tmp;
	} else {
		return __libc_memalign(align, size);
	}
}

void free(void *ptr) {
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
		for(uint i=0;i<n;i++) {
			uint8_t val = pmc_load8(((char *) src) + i, memstring);
			pmc_store8(((char *) dst)+i, val, memstring);
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
void * memmove(void *dst, const void *src, size_t len) {
	if (isPersistent(dst,1) && !inside_model) {
		for(uint i=0;i<len;i++) {
			uint8_t val =pmc_load8(((char *) src) + i, memmovestring);
			pmc_store8(((char *) dst)+i, val, memmovestring);
		}
		return dst;
	} else {
		if (!memmove_real) {
			memmove_real = (void * (*)(void * dst, const void *src, size_t len))dlsym(RTLD_NEXT, "memmove");
		}
		return memmove_real(dst, src, len);
	}
}

void * (*memset_real)(void * dst, int c, size_t len) = NULL;
const char * memsetstring = "memset";
void * memset(void *dst, int c, size_t len) {
	if (isPersistent(dst,1) && !inside_model) {
		for(uint i=0;i<len;i++) {
			pmc_store8(((char *) dst)+i, c, memsetstring);
		}
		return dst;
	} else {
		if (!memset_real) {
			memset_real = (void * (*)(void * dst, int c, size_t len))dlsym(RTLD_NEXT, "memset");
		}
		return memset_real(dst, c, len);
	}
}

void (*bzero_real)(void * dst, size_t len) = NULL;
const char * bzerostring = "bzero";
void bzero(void *dst, size_t len) {
	if (isPersistent(dst,1) && !inside_model) {
		for(uint i=0;i<len;i++) {
			pmc_store8(((char *) dst)+i, 0, memsetstring);
		}
	} else {
		if (!bzero_real) {
			bzero_real = (void (*)(void * dst, size_t len))dlsym(RTLD_NEXT, "bzero");
		}
		bzero_real(dst, len);
	}
}

char * (*strcpy_real)(char * dst, const char *src) = NULL;
const char * strcpystring = "strcpy";
char * strcpy(char *dst, const char *src) {
	if (isPersistent(dst,1) && !inside_model) {
		size_t len = 0;
		while(src[len] != '\0') len++;
		for(uint i=0;i<=len;i++) {
			uint8_t val =pmc_load8(((char *) src) + i, strcpystring);
			pmc_store8(((char *) dst)+i, val, strcpystring);
		}
		return dst;
	} else {
		if (!strcpy_real) {
			strcpy_real = (char * (*)(char * dst, const char *src))dlsym(RTLD_NEXT, strcpystring);
		}
		return strcpy_real(dst, src);
	}
}

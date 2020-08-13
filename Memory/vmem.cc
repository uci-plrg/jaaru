#include "malloc.h"
#include "model.h"
#include "persistentmemory.h"
#include "mymemory.h"


#ifdef ENABLE_VMEM

extern "C" {
typedef void * mspace;
void* __libc_malloc(size_t size);
void* __libc_calloc(size_t n, size_t size);
void* __libc_realloc(void* address, size_t size);
void* __libc_memalign(size_t alignment, size_t size);
void __libc_free(void* ptr);

extern void * mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);
extern void * mspace_realloc(mspace msp, void* mem, size_t newsize);
extern void * mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);
extern void * mspace_memalign(mspace msp, size_t alignment, size_t bytes);
extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
extern mspace create_mspace(size_t capacity, int locked);
}
void * realmemset(void *dst, int c, size_t n);


void *malloc(size_t size) {
	createModelIfNotExist();
	if (mallocSpace) {
		void * tmp = mspace_malloc(mallocSpace, size);
		ASSERT(tmp);
		realmemset(tmp, FILLBYTE, size);
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
		realmemset(tmp, FILLBYTE, size);
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
		realmemset(tmp, FILLBYTE, size);
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

#endif
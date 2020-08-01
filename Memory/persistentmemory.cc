#include "persistentmemory.h"
#include "malloc.h"
#include "config.h"

extern "C" {
void* __libc_malloc(size_t size);
void* __libc_calloc(size_t n, size_t size);
void* __libc_realloc(void* address, size_t size);
void* __libc_memalign(size_t alignment, size_t size);
void __libc_free(void* ptr);
}	// extern "C"

uintptr_t low_mem = 0;
uintptr_t high_mem = 0;


void initializePersistentMemory() {
}

bool isPersistent(void *address, uint size) {
	return (low_mem != 0) && (((uintptr_t)address) >= low_mem &&
														((uintptr_t) address) < high_mem);
}

void *malloc(size_t size) {
	void * result = __libc_malloc(size);
	if (result != NULL) {
		if (((uintptr_t) result) < low_mem || low_mem == 0) {
			low_mem = (uintptr_t)result;
		}
		if ((((uintptr_t) result) + size) > high_mem) {
			high_mem = ((uintptr_t)result) + size;
		}
	}
	return result;
}

void *calloc(size_t count, size_t size) {
	void * result = __libc_calloc(count, size);
	if (result != NULL) {
		if (((uintptr_t) result) < low_mem || low_mem == 0) {
			low_mem = (uintptr_t)result;
		}
		if ((((uintptr_t) result) + size) > high_mem) {
			high_mem = ((uintptr_t)result) + size;
		}
	}
	return result;
}

void *realloc(void * ptr, size_t size) {
	void * result = __libc_realloc(ptr, size);
	if (result != NULL) {
		if (((uintptr_t) result) < low_mem || low_mem == 0) {
			low_mem = (uintptr_t)result;
		}
		if ((((uintptr_t) result) + size) > high_mem) {
			high_mem = ((uintptr_t)result) + size;
		}
	}
	return result;
}

void *memalign(size_t align, size_t size) {
	void * result = __libc_memalign(align, size);
	if (result != NULL) {
		if (((uintptr_t) result) < low_mem || low_mem == 0) {
			low_mem = (uintptr_t)result;
		}
		if ((((uintptr_t) result) + size) > high_mem) {
			high_mem = ((uintptr_t)result) + size;
		}
	}
	return result;
}

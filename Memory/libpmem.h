#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp);
int pmem_file_exists(const char *path);
int pmem_unmap(void *addr, size_t len);
int pmem_is_pmem(const void *addr, size_t len);
void pmem_persist(const void *addr, size_t len);
int pmem_msync(const void *addr, size_t len);
int pmem_has_auto_flush(void);
void pmem_flush(const void *addr, size_t len);
void pmem_deep_flush(const void *addr, size_t len);
int pmem_deep_drain(const void *addr, size_t len);
int pmem_deep_persist(const void *addr, size_t len);
void pmem_drain(void);
int pmem_has_hw_drain(void);

//bogus PMEM_FILE definitions to keep programs happy
#define PMEM_FILE_EXCL 1
#define PMEM_FILE_CREATE 2

#define PMEM_F_MEM_NODRAIN      (1U << 0)

#define PMEM_F_MEM_NONTEMPORAL  (1U << 1)
#define PMEM_F_MEM_TEMPORAL     (1U << 2)

#define PMEM_F_MEM_WC           (1U << 3)
#define PMEM_F_MEM_WB           (1U << 4)

#define PMEM_F_MEM_NOFLUSH      (1U << 5)

#define PMEM_F_MEM_VALID_FLAGS (PMEM_F_MEM_NODRAIN | \
																PMEM_F_MEM_NONTEMPORAL | \
																PMEM_F_MEM_TEMPORAL | \
																PMEM_F_MEM_WC | \
																PMEM_F_MEM_WB | \
																PMEM_F_MEM_NOFLUSH)

void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memset_persist(void *pmemdest, int c, size_t len);

void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memset_nodrain(void *pmemdest, int c, size_t len);

void *pmem_memmove(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memset(void *pmemdest, int c, size_t len, unsigned flags);


void * pmdk_malloc(size_t size);
void *pmdk_pagealigned_calloc(size_t size);
int pmem_register_file(const char *path, void * addr);
void jaaru_enable_simulating_crash(void);
void jaaru_inject_crash(void);
int jaaru_num_crashes();
#ifdef __cplusplus
}
#endif


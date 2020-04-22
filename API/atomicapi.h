/** @file atomicapi.h
 *  @brief C interface for PMCheck atomic operations.
 */

#ifndef ATOMICAPI_H
#define ATOMICAPI_H

#if __cplusplus
extern "C" {
#else
typedef int bool;
#endif

// PMC volatile loads
uint8_t pmc_volatile_load8(void * obj, const char * position);
uint16_t pmc_volatile_load16(void * obj, const char * position);
uint32_t pmc_volatile_load32(void * obj, const char * position);
uint64_t pmc_volatile_load64(void * obj, const char * position);

// PMC volatile stores
void pmc_volatile_store8(void * obj, uint8_t val, const char * position);
void pmc_volatile_store16(void * obj, uint16_t val, const char * position);
void pmc_volatile_store32(void * obj, uint32_t val, const char * position);
void pmc_volatile_store64(void * obj, uint64_t val, const char * position);

// PMC Atomic init
void pmc_atomic_init8(void * obj, uint8_t val, const char * position);
void pmc_atomic_init16(void * obj, uint16_t val, const char * position);
void pmc_atomic_init32(void * obj, uint32_t val, const char * position);
void pmc_atomic_init64(void * obj, uint64_t val, const char * position);

uint8_t  pmc_atomic_load8(void * obj, int atomic_index, const char * position);
uint16_t pmc_atomic_load16(void * obj, int atomic_index, const char * position);
uint32_t pmc_atomic_load32(void * obj, int atomic_index, const char * position);
uint64_t pmc_atomic_load64(void * obj, int atomic_index, const char * position);

void pmc_atomic_store8(void * obj, uint8_t val, int atomic_index, const char * position);
void pmc_atomic_store16(void * obj, uint16_t val, int atomic_index, const char * position);
void pmc_atomic_store32(void * obj, uint32_t val, int atomic_index, const char * position);
void pmc_atomic_store64(void * obj, uint64_t val, int atomic_index, const char * position);


// pmc atomic exchange
uint8_t pmc_atomic_exchange8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_exchange16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_exchange32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_exchange64(void* addr, uint64_t val, int atomic_index, const char * position);
// pmc atomic fetch add
uint8_t  pmc_atomic_fetch_add8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_fetch_add16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_fetch_add32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_fetch_add64(void* addr, uint64_t val, int atomic_index, const char * position);
// pmc atomic fetch sub
uint8_t  pmc_atomic_fetch_sub8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_fetch_sub16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_fetch_sub32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_fetch_sub64(void* addr, uint64_t val, int atomic_index, const char * position);
// pmc atomic fetch and
uint8_t pmc_atomic_fetch_and8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_fetch_and16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_fetch_and32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_fetch_and64(void* addr, uint64_t val, int atomic_index, const char * position);
// pmc atomic fetch or
uint8_t pmc_atomic_fetch_or8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_fetch_or16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_fetch_or32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_fetch_or64(void* addr, uint64_t val, int atomic_index, const char * position);
// pmc atomic fetch xor
uint8_t pmc_atomic_fetch_xor8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t pmc_atomic_fetch_xor16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t pmc_atomic_fetch_xor32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t pmc_atomic_fetch_xor64(void* addr, uint64_t val, int atomic_index, const char * position);

// pmc atomic compare and exchange (strong)
uint8_t pmc_atomic_compare_exchange8_v1(void* addr, uint8_t expected, uint8_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint16_t pmc_atomic_compare_exchange16_v1(void* addr, uint16_t expected, uint16_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint32_t pmc_atomic_compare_exchange32_v1(void* addr, uint32_t expected, uint32_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint64_t pmc_atomic_compare_exchange64_v1(void* addr, uint64_t expected, uint64_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);

bool pmc_atomic_compare_exchange8_v2(void* addr, uint8_t* expected, uint8_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool pmc_atomic_compare_exchange16_v2(void* addr, uint16_t* expected, uint16_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool pmc_atomic_compare_exchange32_v2(void* addr, uint32_t* expected, uint32_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool pmc_atomic_compare_exchange64_v2(void* addr, uint64_t* expected, uint64_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);

// pmc atomic thread fence
void pmc_atomic_thread_fence(int atomic_index, const char * position);

#if __cplusplus
}
#endif

#endif

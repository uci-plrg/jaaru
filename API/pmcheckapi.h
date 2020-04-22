/** @file pmcheckapi.h
 *  @brief C interface to the model checker.
 */

#ifndef PMCHECKAPI_H
#define PMCHECKAPI_H

#include<stdint.h>

#if __cplusplus
extern "C" {
#else
typedef int bool;
#endif
void pmc_load8(const void *addr);
void pmc_load16(const void *addr);
void pmc_load32(const void *addr);
void pmc_load64(const void *addr);

void pmc_store8(void *addr);
void pmc_store16(void *addr);
void pmc_store32(void *addr);
void pmc_store64(void *addr);

// PMC NVM operations
void pmc_clwb(char * addrs);
void pmc_clflushopt(char * addrs);
void pmc_clflush(char * addrs);
void pmc_mfence();
void pmc_lfence();
void pmc_sfence();

#if __cplusplus
}
#endif

#endif

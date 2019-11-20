#ifndef INCLUDE_PFLUSH_H_
#define INCLUDE_PFLUSH_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLFLUSHOPT_USED (0)
#define CLFLUSH_USED (1)
#define CLWB_USED (2)

/*  Cache line flush: 
    clflush can be replaced with clflushopt or clwb, if the CPU supports clflushopt or clwb.  
*/
#define asm_clwb(addr)                      \
    ({                                      \
        __asm__ __volatile__("clwb %0"      \
                             :              \
                             : "m"(*addr)); \
    })

#define asm_clflush(addr)                   \
    ({                                      \
        __asm__ __volatile__("clflush %0"   \
                             :              \
                             : "m"(*addr)); \
    })

#define asm_clflushopt(addr)                 \
    ({                                       \
        __asm__ __volatile__("clflushopt %0" \
                             :               \
                             : "m"(*addr));  \
    })

/*  Memory fence:  
    mfence can be replaced with sfence, if the CPU supports sfence.
*/
#define asm_mfence()                          \
    ({                                        \
        __asm__ __volatile__("mfence" ::      \
                                 : "memory"); \
    })

#define asm_lfence()                          \
    ({                                        \
        __asm__ __volatile__("lfence" ::      \
                                 : "memory"); \
    })

#define asm_sfence()                          \
    ({                                        \
        __asm__ __volatile__("sfence" ::      \
                                 : "memory"); \
    })

void persist_data(int sync, int type, void* data, size_t len);

void nvmem_memcpy(int sync, char *dest, const char *src, size_t len);

unsigned long long asm_rdtsc(void);

unsigned long long asm_rdtscp(void);

uint64_t cycles_to_ns(int cpu_speed_mhz, uint64_t cycles);

#endif
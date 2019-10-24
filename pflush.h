#ifndef INCLUDE_PFLUSH_H_
#define INCLUDE_PFLUSH_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

void pflush(uint64_t *addr);

void init_pflush(int cpu_speed_mhz, int write_latency_ns);

void persist_data(void *data, size_t len);

unsigned long long asm_rdtsc(void);

unsigned long long asm_rdtscp(void);

uint64_t cycles_to_ns(int cpu_speed_mhz, uint64_t cycles);

#endif
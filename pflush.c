#include "pflush.h"

/* 
 *  Note that we refered to the implementation code of pflush function in Quartz
 */

static int global_cpu_speed_mhz = 2200;
static int global_write_latency_ns = 0;

unsigned long long asm_rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

unsigned long long asm_rdtscp(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtscp"
                         : "=a"(lo), "=d"(hi)::"rcx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

void init_pflush(int cpu_speed_mhz, int write_latency_ns)
{
    global_cpu_speed_mhz = cpu_speed_mhz;
    global_write_latency_ns = write_latency_ns;
}

uint64_t cycles_to_ns(int cpu_speed_mhz, uint64_t cycles)
{
    return (cycles * 1000 / cpu_speed_mhz);
}

uint64_t ns_to_cycles(int cpu_speed_mhz, uint64_t ns)
{
    return (ns * cpu_speed_mhz / 1000);
}

static inline int emulate_latency_ns(int ns)
{
    if (ns <= 0) {
        return 0;
    }

    uint64_t cycles;
    uint64_t start;
    uint64_t stop;

    start = asm_rdtsc();
    cycles = ns_to_cycles(global_cpu_speed_mhz, ns);
    do {
        stop = asm_rdtsc();
    } while (stop - start < cycles);

    return 0;
}

/*
Function: pflush() 
        Flush a cache line with the address addr;
*/
void pflush(uint64_t* addr)
{
    /* Measure the latency of a clflush and add an additional delay to
       meet the write latency to NVM 
    */
    uint64_t start;
    uint64_t stop;
    start = asm_rdtscp();
    asm_clflush(addr);
    stop = asm_rdtscp();
    emulate_latency_ns(global_write_latency_ns - cycles_to_ns(global_cpu_speed_mhz, stop - start));
}

static inline void mfence()
{
    asm volatile("mfence" ::
                     : "memory");
}

static inline void sfence()
{
    asm volatile("sfence" ::
                     : "memory");
}

static inline void clflush(volatile void* __p)
{
    asm volatile("clflush (%0)" ::"r"(__p));
}

static inline void cpu_pause()
{
    __asm__ volatile("pause" ::
                         : "memory");
}

static inline unsigned long read_tsc(void)
{
    unsigned long var;
    unsigned int hi, lo;

    asm volatile("rdtsc"
                 : "=a"(lo), "=d"(hi));
    var = ((unsigned long long int)hi << 32) | lo;
    return var;
}

void persist_data(void* _data, size_t len)
{
    volatile char* data = (char*)_data;
    volatile char* ptr = (char*)((unsigned long)data & ~(64 - 1));
    for (; ptr < data + len; ptr += 64) {
        asm_clflush((uint64_t*)ptr);
        // asm_clwb((uint64_t *)ptr);
        // asm_clflushopt((uint64_t *)ptr);
    }
    asm_mfence();
}
#include "libpmem.h"
#include "ntstore.h"
#include "pflush.h"
#include "timer.h"
#include <assert.h>
#include <pthread.h>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <vector>

// #include <fcntl.h>
// #include <sys/mman.h>

#define ASSERT_VERIFY
#define THREAD_BIND_CPU
#define PM_USED
#define PMDK_USED
// #define NO_ALIGN
#define RANDOM_SKIP (1024)
#define PMEM_POOL_SIZE ((size_t)450 * 1024 * 1024 * 1024)

static double run_seconds = 25.0;
static int use_clock = 0;
static int time_is_over = 0;

#define R_WRITE (0)
#define S_WRITE (1)
#define R_READ (2)
#define S_READ (3)
#define S_MIXED (4)
#define R_MIXED (5)
#define ALIGN_SIZE (256)

struct test_result {
    uint64_t time;
    uint64_t latency;
    uint64_t throughput;
    size_t count;
};

struct thread_options {
    int id;
    int type;
    uint64_t addr_start;
    uint64_t addr_end;
    size_t write_amount;
    size_t read_amount;
    size_t block_size;
    size_t count;
    struct test_result* result;
};

struct benchmark_options {
    int type;
    int num_thread;
    int read_thread;
    int write_thread;
    uint64_t pmem_start_address;
    size_t block_size;
    size_t opt_count;
    size_t data_amount;
    size_t pmem_size;
};

struct block_node {
    uint64_t next_block_id;
};

uint64_t pm_init(const char* pool_name, size_t& pmem_size)
{
    size_t pm_size;
    uint64_t pmem_start_address;
#ifdef PM_USED
#ifdef PMDK_USED
    size_t mapped_len;
    int is_pmem;
    /* create a new memory pool file */
    void* dest = pmem_map_file(pool_name, 0, 0, 0666, &mapped_len, &is_pmem);
    // void *dest = pmem_map_file(pool_name, PMEM_POOL_SIZE, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
    pm_size = mapped_len;
    printf("Use PMDK to mmap file! (%s) (%zuMB/%zuMB) (%d)\n", pool_name, pm_size / (1024 * 1024), mapped_len / (1024 * 1024), is_pmem);
#else
    int fd = open(pool_name, O_RDWR);
    void* dest = mmap(NULL, pmem_size, PROT_READ | PROT_WRITE, MAP_SHARED /*| MAP_POPULATE*/, fd, 0);
    pm_size = pmem_size;
#endif
    assert(dest != NULL);
    pmem_start_address = (uint64_t)dest;
    printf("Persistent Memory Pool Start Address : 0x%llx\n", (uint64_t)dest);
#else
    pmem_start_address = (uint64_t)malloc(pmem_size);
    assert(pmem_start_address != 0);
    pm_size = pmem_size;
    printf("DRAM Pool Start Address : 0x%llx\n", (uint64_t)pmem_start_address);
#endif
    pmem_size = pm_size;
    return pmem_start_address;
}

void randomwrite(struct thread_options* opt, struct test_result* result)
{
    Timer timer;
    int id = opt->id;
    uint64_t address = (uint64_t)opt->addr_start;
    size_t block_size = opt->block_size;
    uint64_t sum_time = 0;
    size_t count = opt->write_amount / opt->block_size;
    result->count = count;
    size_t run_count = 0;
    uint8_t* data = (uint8_t*)malloc(block_size);

    if (address % ALIGN_SIZE != 0) {
        address += ALIGN_SIZE;
        address &= (~((uint64_t)ALIGN_SIZE - 1));
        assert(address % ALIGN_SIZE == 0);
    }

    size_t skip_step = block_size < RANDOM_SKIP ? RANDOM_SKIP : block_size + RANDOM_SKIP;
    timer.Start();

    for (size_t i = 0; i < count; i++) {
        // memcpy((void *)addr, (void *)data, block_size);
        // persist_data((void *)addr, block_size);
        nvmem_memcpy((char*)address, (char*)data, block_size);
        address += skip_step; // skip 256B

        if ((uint64_t)address >= (uint64_t)opt->addr_end) {
            address = (uint64_t)opt->addr_start;
            if (address % ALIGN_SIZE != 0) {
                address += ALIGN_SIZE;
                address &= (~((uint64_t)ALIGN_SIZE - 1));
                assert(address % ALIGN_SIZE == 0);
            }
        }

#ifdef ASSERT_VERIFY
        assert(address % ALIGN_SIZE == 0); // must align with 256B
#endif

        if (use_clock) {
            run_count++;
            if (time_is_over) {
                result->count = run_count;
                break;
            } else {
                i = 0;
            }
        }
    }
    
    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * block_size / (1024 * 1024);
    free(data);
}

void seqwrite(struct thread_options* opt, struct test_result* result)
{
    Timer timer;
    int id = opt->id;
    uint64_t address = (uint64_t)opt->addr_start;
    size_t block_size = opt->block_size;
    uint64_t sum_time = 0;
    // size_t count = opt->count;
    size_t count = opt->write_amount / opt->block_size;
    result->count = count;
    size_t run_count = 0;
    uint8_t* data = (uint8_t*)malloc(block_size);

    if (address % ALIGN_SIZE != 0) {
        address += ALIGN_SIZE;
        address &= (~((uint64_t)ALIGN_SIZE - 1));
        assert(address % ALIGN_SIZE == 0);
    }

    timer.Start();

    for (size_t i = 0; i < count; i++) {

        // memcpy((void *)address, (void *)data, block_size);
        // persist_data((void *)address, block_size);
        nvmem_memcpy((char*)address, (char*)data, block_size);

        address += block_size; // seq

        if ((uint64_t)address >= (uint64_t)opt->addr_end) {
            address = (uint64_t)opt->addr_start;
            if (address % ALIGN_SIZE != 0) {
                address += ALIGN_SIZE;
                address &= (~((uint64_t)ALIGN_SIZE - 1));
                assert(address % ALIGN_SIZE == 0);
            }
        }

        if (use_clock) {
            run_count++;
            if (time_is_over) {
                result->count = run_count;
                break;
            } else {
                i = 0; // while(1)
            }
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * block_size / (1024 * 1024);
    free(data);
}

void randomread(struct thread_options* opt, struct test_result* result)
{
    Timer timer;
    int id = opt->id;
    uint64_t address = (uint64_t)opt->addr_start;
    uint64_t address_end = (uint64_t)opt->addr_end;
    size_t block_size = opt->block_size;
    uint64_t sum_time = 0;
    // size_t count = opt->count;
    size_t count = opt->write_amount / opt->block_size;
    result->count = count;
    size_t run_count = 0;
    uint8_t* data = (uint8_t*)malloc(block_size);
    memset(data, 0, block_size);

    if (address % ALIGN_SIZE != 0) {
        address += ALIGN_SIZE;
        address &= (~((uint64_t)ALIGN_SIZE - 1));
        assert(address % ALIGN_SIZE == 0);
    }

    size_t skip_step = block_size < RANDOM_SKIP ? RANDOM_SKIP : block_size + RANDOM_SKIP;
    timer.Start();

    for (size_t i = 0; i < count; i++) {

        memcpy((void*)data, (void*)address, block_size);
        asm_lfence();
        address += skip_step; // skip 1KB

        if ((uint64_t)address > (uint64_t)opt->addr_end) {
            address = (uint64_t)opt->addr_start;
            if (address % ALIGN_SIZE != 0) {
                address += ALIGN_SIZE;
                address &= (~((uint64_t)ALIGN_SIZE - 1));
                assert(address % ALIGN_SIZE == 0);
            }
        }

#ifdef ASSERT_VERIFY
        assert(address % ALIGN_SIZE == 0); // must align with 256B
#endif

        if (use_clock) {
            run_count++;
            if (time_is_over) {
                result->count = run_count;
                break;
            } else {
                i = 0; // while(1)
            }
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * block_size / (1024 * 1024);
    free(data);
}

void seqread(struct thread_options* opt, struct test_result* result)
{
    Timer timer;
    int id = opt->id;
    uint64_t address = (uint64_t)opt->addr_start;
    size_t block_size = opt->block_size;
    uint64_t sum_time = 0;
    size_t run_count = 0;
    // size_t count = opt->count; // opt->write_amount / opt->block_size;
    size_t count = opt->write_amount / opt->block_size;
    result->count = count;
    uint8_t* data = (uint8_t*)malloc(block_size);

    if (address % ALIGN_SIZE != 0) {
        address += ALIGN_SIZE;
        address &= (~((uint64_t)ALIGN_SIZE - 1));
        assert(address % ALIGN_SIZE == 0);
    }

    timer.Start();

    for (size_t i = 0; i < count; i++) {
        memcpy((void*)data, (void*)address, block_size);
        asm_lfence();
        address += block_size;

        if ((uint64_t)address >= (uint64_t)opt->addr_end) {
            address = (uint64_t)opt->addr_start;
            if (address % ALIGN_SIZE != 0) {
                address += ALIGN_SIZE;
                address &= (~((uint64_t)ALIGN_SIZE - 1));
                assert(address % ALIGN_SIZE == 0);
            }
        }

        if (use_clock) {
            run_count++;
            if (time_is_over) {
                result->count = run_count;
                break;
            } else {
                i = 0;
            }
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * block_size / (1024 * 1024);
    free(data);
}

void* thread_task(void* opt)
{
    struct thread_options* options = (struct thread_options*)opt;
    int id = options->id;
    uint64_t addr_start = options->addr_start;
    uint64_t addr_end = options->addr_end;

#ifdef THREAD_BIND_CPU
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(id + 1, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }
#endif

    switch (options->type) {
    case R_READ:
        randomread(options, options->result);
        break;
    case S_READ:
        seqread(options, options->result);
        break;
    case R_WRITE:
        randomwrite(options, options->result);
        break;
    case S_WRITE:
        seqwrite(options, options->result);
        break;
    default:
        printf("error switch!\n");
        break;
    }

    printf("  [Thread%2d][Count:%zu][Time:%lluseconds][Latency:%lluns][Throughput:%lluMB/s]\n",
        id, options->result->count, options->result->time,
        options->result->latency, options->result->throughput);
    return NULL;
}

void run_benchmark(const char name[], struct benchmark_options* opt)
{
    pthread_t thread_id[32];
    int type = opt->type;
    int num_thread = opt->num_thread;
    size_t block_size = opt->block_size;
    size_t data_amount = opt->data_amount;
    size_t pmem_size = opt->pmem_size;
    uint64_t pmem_start_address = opt->pmem_start_address;
    uint64_t addr = pmem_start_address;
    size_t partition_size = opt->pmem_size / opt->num_thread;

    // time thread
#ifdef THREAD_BIND_CPU
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }
#endif

    printf(">>[Running Benchmark]\n");
    printf("  [%s][Block Size:%zuB][Data Amount:%zuMB]\n", name, block_size, data_amount / (1024 * 1024));

    struct thread_options opts[32];
    struct test_result res[32];

    for (int i = 0; i < num_thread; i++) {
        opts[i].id = i;
        opts[i].type = type;
        opts[i].addr_start = addr;
        /* we need to ensutre the start address is align 16B */
        if (opts[i].addr_start % ALIGN_SIZE != 0) {
            opts[i].addr_start += ALIGN_SIZE;
            opts[i].addr_start &= (~((uint64_t)ALIGN_SIZE - 1));
        }
        assert(opts[i].addr_start % ALIGN_SIZE == 0);
        opts[i].addr_end = addr + data_amount;
        opts[i].block_size = block_size;
        opts[i].result = &res[i];
        opts[i].read_amount = data_amount;
        opts[i].write_amount = data_amount;
        opts[i].count = opt->opt_count;
        addr += partition_size;
        printf("  [Partition%d][addr:0x%llx/%llu][size:%zuMB]\n", i, (uint64_t)addr, (uint64_t)addr, partition_size / (1024 * 1024));
        pthread_create(thread_id + i, NULL, thread_task, (void*)&opts[i]);
    }

    for (int i = 0; i < num_thread; i++) {
        pthread_join(thread_id[i], NULL);
    }

    uint64_t avg_latency = 0;
    uint64_t total_throughput = 0;

    for (int i = 0; i < num_thread; i++) {
        avg_latency += res[i].latency;
        total_throughput += res[i].throughput;
    }
    avg_latency /= num_thread;
    printf("  [SUM][Latency:%lluns][Throughtput:%lluMB/s]\n", avg_latency, total_throughput);
}

void run_mixed_benchmark(const char name[], struct benchmark_options* opt)
{
    pthread_t thread_id[32];
    int type = opt->type;
    int num_thread = opt->read_thread + opt->write_thread;
    opt->num_thread = num_thread;
    size_t block_size = opt->block_size;
    size_t data_amount = opt->data_amount;
    size_t pmem_size = opt->pmem_size;
    uint64_t pmem_start_address = opt->pmem_start_address;
    uint64_t addr = pmem_start_address;
    size_t partition_size = opt->pmem_size / opt->num_thread;

    printf(">>[Running Mixed Benchmark]\n");
    printf("  [%s][Block Size:%zuB][Data Amount:%zuMB]\n", name, block_size, data_amount / (1024 * 1024));

    struct thread_options opts[32];
    struct test_result res[32];

    // time thread
#ifdef THREAD_BIND_CPU
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        printf("threadpool, set thread affinity failed.\n");
    }
#endif

    use_clock = 1;
    time_is_over = 0;

    for (int i = 0; i < num_thread; i++) {
        opts[i].id = i;
        if (i < opt->read_thread) {
            opts[i].type = (type == S_MIXED) ? S_READ : R_READ;
        } else {
            opts[i].type = (type == S_MIXED) ? S_WRITE : R_WRITE;
        }
        opts[i].addr_start = addr;
        /* we need to ensutre the start address is align 16B */
        if (opts[i].addr_start % ALIGN_SIZE != 0) {
            opts[i].addr_start += ALIGN_SIZE;
            opts[i].addr_start &= (~((uint64_t)ALIGN_SIZE - 1));
        }
        assert(opts[i].addr_start % ALIGN_SIZE == 0);
        opts[i].addr_end = addr + data_amount;
        opts[i].block_size = block_size;
        opts[i].result = &res[i];
        opts[i].read_amount = data_amount;
        opts[i].write_amount = data_amount;
        opts[i].count = opt->opt_count;
        addr += partition_size;
        printf("  [Partition%d][addr:0x%llx/%llu][size:%zuMB]\n", i, (uint64_t)addr, (uint64_t)addr, partition_size / (1024 * 1024));
        pthread_create(thread_id + i, NULL, thread_task, (void*)&opts[i]);
    }

    Timer global_clock;
    global_clock.Start();

    printf(">>Clock Start!\n");
    while (true) {
        global_clock.Stop();
        if (global_clock.GetSeconds() > run_seconds) {
            time_is_over = true;
            break;
        }
    }
    printf(">>Clock End! (%.2f)\n", global_clock.GetSeconds());

    for (int i = 0; i < num_thread; i++) {
        pthread_join(thread_id[i], NULL);
    }

    uint64_t avg_read_latency = 0;
    uint64_t avg_write_latency = 0;
    uint64_t total_read_throughput = 0;
    uint64_t total_write_throughput = 0;

    for (int i = 0; i < num_thread; i++) {
        if (i < opt->read_thread) {
            avg_read_latency += res[i].latency;
            total_read_throughput += res[i].throughput;
        } else {
            avg_write_latency += res[i].latency;
            total_write_throughput += res[i].throughput;
        }
    }

    if (opt->read_thread > 0) {
        avg_read_latency /= opt->read_thread;
    }
    if (opt->write_thread > 0) {
        avg_write_latency /= opt->write_thread;
    }

    printf("  [READ][Latency:%lluns][Throughtput:%lluMB/s]\n", avg_read_latency, total_read_throughput);
    printf("  [WRITE][Latency:%lluns][Throughtput:%lluMB/s]\n", avg_write_latency, total_write_throughput);
    printf("  [SUM][Throughtput:%lluMB/s]\n", total_read_throughput + total_write_throughput);
}

int main(int argc, char* argv[])
{
    char test_benchmark[128] = "sw"; // sw sr rr
    struct benchmark_options options;
    options.num_thread = 1;
    options.block_size = 128;
    options.data_amount = 128 * 1024 * 1024;
    options.pmem_size = (size_t)2048 * 1024 * 1024;

    for (int i = 0; i < argc; i++) {
        char junk;
        uint64_t n;
        if (sscanf(argv[i], "--block_size=%llu%c", &n, &junk) == 1) {
            options.block_size = n;
        } else if (sscanf(argv[i], "--num_thread=%llu%c", &n, &junk) == 1) {
            options.num_thread = n;
        } else if (sscanf(argv[i], "--data_amount=%llu%c", &n, &junk) == 1) {
            options.data_amount = n * 1024 * 1024;
        } else if (sscanf(argv[i], "--read_thread=%llu%c", &n, &junk) == 1) {
            options.read_thread = n;
        } else if (sscanf(argv[i], "--write_thread=%llu%c", &n, &junk) == 1) {
            options.write_thread = n;
        } else if (sscanf(argv[i], "--count=%llu%c", &n, &junk) == 1) {
            options.opt_count = n;
        } else if (strncmp(argv[i], "--benchmark=", 12) == 0) {
            strcpy(test_benchmark, argv[i] + 12);
        } else if (i > 0) {
            printf("error (%s)!\n", argv[i]);
            return 0;
        }
    }

    options.pmem_start_address = pm_init("/home/pmem0/pm", options.pmem_size);
    assert(options.pmem_size >= ((size_t)options.num_thread * options.data_amount));

    if (memcmp(test_benchmark, "rw", 2) == 0) {
        options.type = R_WRITE;
        run_benchmark("randomwrite", &options);
    }
    if (memcmp(test_benchmark, "rr", 2) == 0) {
        options.type = R_READ;
        run_benchmark("randomread", &options);
    }
    if (memcmp(test_benchmark, "sr", 2) == 0) {
        options.type = S_READ;
        run_benchmark("seqread", &options);
    }
    if (memcmp(test_benchmark, "sw", 2) == 0) {
        options.type = S_WRITE;
        run_benchmark("seqwrite", &options);
    }
    if (memcmp(test_benchmark, "sm", 2) == 0) {
        options.type = S_MIXED;
        run_mixed_benchmark("seq mixed", &options);
    }
    if (memcmp(test_benchmark, "rm", 2) == 0) {
        options.type = R_MIXED;
        run_mixed_benchmark("random mixed", &options);
    }

    return 0;
}
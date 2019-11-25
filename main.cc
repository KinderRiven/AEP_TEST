#include "libpmem.h"
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
#define THREAD_BIND_CPU
#define PM_USED
#define PMDK_USED
#define NUMA0
#define RANDOM_SKIP (1024)

// CPU core bind
static int numa_bind[2][20] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 },
    { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 } };

static double run_seconds = 30.0; // read-write workloads
static int clock_used = 0; // only used in read-write mixed workloads
static int time_is_over = 0;

#define R_WRITE (0)
#define S_WRITE (1)
#define R_READ (2)
#define S_READ (3)
#define S_MIXED (4)
#define R_MIXED (5)

struct test_result {
    uint64_t time;
    uint64_t latency;
    uint64_t throughput;
    size_t count;
};

struct thread_options {
    int id; // thread id
    int type; // benchmark type
    int sync; // clflush + persist
    int flush; // flush type
    int numa;
    int verify; // use assert()
    int align_size; // align size
    int ntstore_used; // ntstore for write
    int clock_used; // clock or count
    uint64_t start_addr; // start pmem address
    uint64_t end_addr; // end pmem address
    size_t write_amount; // write amount
    size_t read_amount; // read amount
    size_t block_size; // block size
    size_t count; // count
    size_t run_timer; // timer (if use clock)
    struct test_result* result;
};

struct benchmark_options {
    int type;
    int sync;
    int numa;
    int flush;
    int verify;
    int align_size;
    int ntstore_used;
    int num_thread;
    int read_thread;
    int write_thread;
    uint64_t pmem_start_address;
    size_t block_size;
    size_t opt_count;
    size_t data_amount;
    size_t pmem_size;
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

inline static void set_zero(void* start_address, void* end_address)
{
    uint8_t* v = (uint8_t*)start_address;
    while (v != (uint8_t *)end_address) {
        *v = 0xa5;
        v++;
    }
    return;
}

void randomwrite(struct thread_options* options, struct test_result* result)
{
    Timer timer;
    uint8_t* buffer = (uint8_t*)malloc(options->block_size); // memory buffer
    uint64_t address = options->start_addr;
    uint64_t sum_time = 0;
    uint64_t total_count = options->write_amount / options->block_size;
    uint64_t finished_count = 0;
    uint64_t skip_step = options->block_size < RANDOM_SKIP ? RANDOM_SKIP : options->block_size + RANDOM_SKIP;

    result->count = total_count;

    if (address % options->align_size != 0) {
        address += options->align_size;
        address /= options->align_size;
        address *= options->align_size;
        assert(address % options->align_size == 0);
    }

    timer.Start();

    for (size_t i = 0; i < total_count;) {

        if (options->ntstore_used) {
            nvmem_memcpy(options->sync, (char*)address, (char*)buffer, options->block_size);
            address += skip_step;
        } else {
            memcpy((void*)address, (void*)buffer, options->block_size);
            persist_data(options->sync, options->flush, (void*)address, options->block_size);
            address += skip_step;
        }

        if (address >= options->end_addr) {
            address = options->start_addr;
            if (address % options->align_size != 0) {
                address += options->align_size;
                address /= options->align_size;
                address *= options->align_size;
                assert(address % options->align_size == 0);
            }
        }

        if (options->verify) {
            assert(address % options->align_size == 0);
        }

        if (options->clock_used) {
            finished_count++;
            if (time_is_over) {
                result->count = finished_count;
                break;
            }
        } else {
            i++;
            finished_count++;
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * options->block_size / (1024 * 1024);
    free(buffer);
}

void seqwrite(struct thread_options* options, struct test_result* result)
{
    Timer timer;
    uint8_t* buffer = (uint8_t*)malloc(options->block_size); // memory buffer
    uint64_t address = options->start_addr;
    uint64_t sum_time = 0;
    uint64_t total_count = options->write_amount / options->block_size;
    uint64_t finished_count = 0;

    result->count = total_count;

    if (address % options->align_size != 0) {
        address += options->align_size;
        address /= options->align_size;
        address *= options->align_size;
        assert(address % options->align_size == 0);
    }

    timer.Start();

    for (size_t i = 0; i < total_count;) {

        if (options->ntstore_used) {
            nvmem_memcpy(options->sync, (char*)address, (char*)buffer, options->block_size);
            address += options->block_size;
        } else {
            memcpy((void*)address, (void*)buffer, options->block_size);
            persist_data(options->sync, options->flush, (void*)address, options->block_size);
            address += options->block_size;
        }

        if (address >= options->end_addr) {
            address = options->start_addr;
            if (address % options->align_size != 0) {
                address += options->align_size;
                address /= options->align_size;
                address *= options->align_size;
                assert(address % options->align_size == 0);
            }
        }

        if (options->clock_used) {
            finished_count++;
            if (time_is_over) {
                result->count = finished_count;
                break;
            }
        } else {
            i++;
            finished_count++;
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * options->block_size / (1024 * 1024);
    free(buffer);
}

void randomread(struct thread_options* options, struct test_result* result)
{
    Timer timer;
    uint8_t* buffer = (uint8_t*)malloc(options->block_size);
    uint64_t address = options->start_addr;
    uint64_t sum_time = 0;
    uint64_t total_count = options->read_amount / options->block_size;
    uint64_t finished_count = 0;
    uint64_t skip_step = options->block_size < RANDOM_SKIP ? RANDOM_SKIP : options->block_size + RANDOM_SKIP;

    result->count = total_count;

    if (address % options->align_size != 0) {
        address += options->align_size;
        address /= options->align_size;
        address *= options->align_size;
        assert(address % options->align_size == 0);
    }

    timer.Start();

    printf("[[%d]]\n", options->block_size);

    for (size_t i = 0; i < total_count;) {
        memcpy((void*)buffer, (void*)address, options->block_size);
        // asm_lfence();
        address += skip_step;

        if (address >= options->end_addr) {
            address = options->start_addr;

            if (address % options->align_size != 0) {
                address += options->align_size;
                address /= options->align_size;
                address *= options->align_size;
                assert(address % options->align_size == 0);
            }
        }

        if (options->verify) {
            assert(address % options->align_size == 0);
        }

        if (options->clock_used) {
            finished_count++;
            if (time_is_over) {
                result->count = finished_count;
                break;
            }
        } else {
            i++;
            finished_count++;
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * options->block_size / (1024 * 1024);
    free(buffer);
}

void seqread(struct thread_options* options, struct test_result* result)
{
    Timer timer;
    uint8_t* buffer = (uint8_t*)malloc(options->block_size);
    uint64_t address = options->start_addr;
    uint64_t sum_time = 0;
    uint64_t total_count = options->read_amount / options->block_size;
    uint64_t finished_count = 0;

    result->count = total_count;

    if (address % options->align_size != 0) {
        address += options->align_size;
        address /= options->align_size;
        address *= options->align_size;
        assert(address % options->align_size == 0);
    }

    timer.Start();

    for (size_t i = 0; i < total_count;) {
        memcpy((void*)buffer, (void*)address, options->block_size);
        // asm_lfence();
        address += options->block_size;

        if (address >= options->end_addr) {
            address = options->start_addr;
            if (address % options->align_size != 0) {
                address += options->align_size;
                address /= options->align_size;
                address *= options->align_size;
                assert(address % options->align_size == 0);
            }
        }

        if (options->clock_used) {
            finished_count++;
            if (time_is_over) {
                result->count = finished_count;
                break;
            }
        } else {
            i++;
            finished_count++;
        }
    }

    timer.Stop();
    sum_time = timer.Get();
    result->time = sum_time;
    result->latency = result->time / result->count;
    result->throughput = (1000000000 / result->latency) * options->block_size / (1024 * 1024);
    free(buffer);
}

void* thread_task(void* opt)
{
    struct thread_options* options = (struct thread_options*)opt;
    int id = options->id;
    uint64_t start_addr = options->start_addr;
    uint64_t end_addr = options->end_addr;

#ifdef THREAD_BIND_CPU
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(numa_bind[options->numa][id], &mask);
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

    printf("  [Thread%2d][Type:%d][Count:%zu][Time:%lluseconds][Latency:%lluns][Throughput:%lluMB/s]\n",
        id, options->type, options->result->count, options->result->time / 1000000000,
        options->result->latency, options->result->throughput);
    return NULL;
}

void run_benchmark(const char name[], struct benchmark_options* opt)
{
    pthread_t thread_id[32];
    int numa = opt->numa;
    int type = opt->type;
    int sync = opt->sync;
    int flush = opt->flush;
    int num_thread = opt->num_thread;
    size_t block_size = opt->block_size;
    size_t data_amount = opt->data_amount;
    size_t pmem_size = opt->pmem_size;
    uint64_t pmem_start_address = opt->pmem_start_address;
    uint64_t addr = pmem_start_address;
    size_t partition_size = opt->pmem_size / opt->num_thread;

    printf(">>[Ready to run basic workloads]\n");
    printf("  [%s][Numa%d][Sync:%d][Flush:%d][Align:%d][Block:%zuB][Data:%zuMB]\n", name, opt->numa, opt->sync, opt->flush, opt->align_size, block_size, data_amount / (1024 * 1024));

    struct thread_options opts[32];
    struct test_result res[32];

    for (int i = 0; i < num_thread; i++) {
        opts[i].id = i;
        opts[i].type = type;
        opts[i].numa = numa;
        opts[i].sync = sync;
        opts[i].flush = flush;
        opts[i].clock_used = 0;
        opts[i].verify = opt->verify;
        opts[i].align_size = opt->align_size;
        opts[i].ntstore_used = opt->ntstore_used;
        opts[i].start_addr = addr;
        opts[i].end_addr = addr + data_amount;
        opts[i].block_size = block_size;
        opts[i].result = &res[i];
        opts[i].read_amount = data_amount;
        opts[i].write_amount = data_amount;
        opts[i].count = opt->opt_count;
        addr += partition_size;
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
    printf("  [SUM][Latency:%lluns][Throughput:%lluMB/s]\n", avg_latency, total_throughput);
}

void run_mixed_benchmark(const char name[], struct benchmark_options* opt)
{
    pthread_t thread_id[32];
    int type = opt->type;
    int numa = opt->numa;
    int sync = opt->sync;
    int flush = opt->flush;
    int num_thread = opt->read_thread + opt->write_thread;
    opt->num_thread = num_thread;
    size_t block_size = opt->block_size;
    size_t data_amount = opt->data_amount;
    size_t pmem_size = opt->pmem_size;
    uint64_t pmem_start_address = opt->pmem_start_address;
    uint64_t addr = pmem_start_address;
    size_t partition_size = opt->pmem_size / opt->num_thread;

    printf(">>[Ready to run read-write mixed workloads]\n");
    printf("  [%s][Numa_%d][Sync:%d][Flush:%d][Align:%d][Block:%zuB][Data:%zuMB]\n", name, opt->numa, opt->sync, opt->flush, opt->align_size, block_size, data_amount / (1024 * 1024));

    struct thread_options opts[32];
    struct test_result res[32];

    clock_used = 1;
    time_is_over = 0;

    for (int i = 0; i < num_thread; i++) {
        opts[i].id = i;
        opts[i].sync = sync;
        opts[i].numa = numa;
        opts[i].flush = flush;
        opts[i].clock_used = 1;
        opts[i].verify = opt->verify;
        opts[i].align_size = opt->align_size;
        opts[i].ntstore_used = opt->ntstore_used;
        if (i < opt->read_thread) {
            opts[i].type = (type == S_MIXED) ? S_READ : R_READ;
        } else {
            opts[i].type = (type == S_MIXED) ? S_WRITE : R_WRITE;
        }
        opts[i].start_addr = addr;
        opts[i].end_addr = addr + data_amount;
        opts[i].block_size = block_size;
        opts[i].result = &res[i];
        opts[i].read_amount = data_amount;
        opts[i].write_amount = data_amount;
        opts[i].count = opt->opt_count;
        addr += partition_size;
        pthread_create(thread_id + i, NULL, thread_task, (void*)&opts[i]);
    }

    Timer global_clock;
    global_clock.Start();

    while (true) {
        global_clock.Stop();
        if (global_clock.GetSeconds() > run_seconds) {
            time_is_over = true;
            break;
        }
    }

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

    printf("  [READ][Latency:%lluns][Throughput:%lluMB/s]\n", avg_read_latency, total_read_throughput);
    printf("  [WRITE][Latency:%lluns][Throughput:%lluMB/s]\n", avg_write_latency, total_write_throughput);
    printf("  [SUM][Throughput:%lluMB/s]\n", total_read_throughput + total_write_throughput);
}

int main(int argc, char* argv[])
{
    char test_benchmark[128] = "sw"; // sw sr rr
    char flush_type[128] = "clflushopt"; // sw sr rr
    char pmem_file_path[128] = "/home/pmem0/pm";
    struct benchmark_options options;
    options.num_thread = 1;
    options.block_size = 128;
    options.data_amount = 128 * 1024 * 1024;
    options.pmem_size = (size_t)2048 * 1024 * 1024;
    options.align_size = 256;
    options.verify = 0;
    options.ntstore_used = 1;
    options.sync = 1;
    options.numa = 0;
    options.flush = CLFLUSHOPT_USED;

    for (int i = 0; i < argc; i++) {
        char junk;
        uint64_t n;
        if (sscanf(argv[i], "--block_size=%llu%c", &n, &junk) == 1) {
            options.block_size = n;
        } else if (sscanf(argv[i], "--num_thread=%llu%c", &n, &junk) == 1) {
            options.num_thread = n;
        } else if (sscanf(argv[i], "--verify=%llu%c", &n, &junk) == 1) {
            options.verify = (n == 0) ? 0 : 1;
        } else if (sscanf(argv[i], "--sync=%llu%c", &n, &junk) == 1) {
            options.sync = (n == 0) ? 0 : 1;
        } else if (sscanf(argv[i], "--numa=%llu%c", &n, &junk) == 1) {
            options.numa = (n == 0) ? 0 : 1;
        } else if (sscanf(argv[i], "--align_size=%llu%c", &n, &junk) == 1) {
            options.align_size = n;
        } else if (sscanf(argv[i], "--ntstore=%llu%c", &n, &junk) == 1) {
            options.ntstore_used = (n == 0) ? 0 : 1;
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
        } else if (strncmp(argv[i], "--flush=", 8) == 0) {
            strcpy(flush_type, argv[i] + 8);
        } else if (strncmp(argv[i], "--pmem_file_path=", 17) == 0) {
            strcpy(pmem_file_path, argv[i] + 17);
        } else if (i > 0) {
            printf("error (%s)!\n", argv[i]);
            return 0;
        }
    }

    if (strcmp(flush_type, "clflush") == 0) {
        options.flush = CLFLUSH_USED;
    } else if (strcmp(flush_type, "clwb") == 0) {
        options.flush = CLWB_USED;
    } else if (strcmp(flush_type, "clflushopt") == 0) {
        options.flush = CLFLUSHOPT_USED;
    }

    options.pmem_start_address = pm_init(pmem_file_path, options.pmem_size);
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
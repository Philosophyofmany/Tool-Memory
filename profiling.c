#define _GNU_SOURCE
#include "profiling.h"
#include "user_code.h"
#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <papi.h>
#include <time.h>

volatile char *array;  // Global array, type matches declaration in profiling.h

uint64_t rdtsc_start() {
    return __rdtsc();
}

double get_cpu_frequency() {
    FILE *fp;
    char buffer[256];
    double cpu_mhz = 0.0;

    // Open the /proc/cpuinfo file
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return 0.0; // Return 0 on failure
    }

    // Read through the file to find the "cpu MHz" entry
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "cpu MHz : %lf", &cpu_mhz) == 1) {
            fclose(fp);
            return cpu_mhz * 1e6; // Convert MHz to Hz
        }
    }

    fclose(fp);
    return 0.0; // Return 0 if not found
}


void set_cpu_affinity(int cpu_id) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);         // Clear all CPUs from the set
    CPU_SET(cpu_id, &cpu_set);  // Add the desired CPU to the set

    // Set CPU affinity for the current process
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
        perror("sched_setaffinity");
        exit(1);
    } else {
        printf("Successfully set CPU affinity to core %d\n", cpu_id);
    }
}


void verify_cpu_affinity() {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);  // Initialize CPU set

    // Get the CPU affinity mask of the current process
    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
        perror("sched_getaffinity");
        exit(1);
    }

    printf("Current CPU affinity: ");
    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &cpu_set)) {
            printf("CPU %d ", i);  // Print all CPUs the process is allowed to run on
        }
    }
    printf("\n");
}


uint64_t rdtsc_end() {
    return __rdtsc();
}

void initialize_memory(size_t size) {
    array = (volatile char *)malloc(size);
    if (array == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    // Warm-up loop to initialize memory and avoid cold-cache effects
    for (size_t i = 0; i < size; i++) {
        array[i] = (char)(i & 0xFF);
    }
}


double measure_bandwidth(size_t block_size, double read_ratio, size_t total_size) {
    uint64_t start, end;
    uint64_t total_cycles = 0;
    double cpu_frequency = get_cpu_frequency(); // Get the CPU frequency dynamically
    // // Print the CPU frequency
    // printf("CPU Frequency: %.2f GHz\n", cpu_frequency / 1e9);
    size_t iterations = 100;  // Number of iterations for averaging
    volatile char temp;
    char *array = malloc(total_size); // Allocate memory for the array

    // Check if allocation succeeded
    if (array == NULL) {
        perror("Failed to allocate memory");
        return 0.0; // Return 0 on failure
    }

    // Calculate read and write counts based on the specified ratio
    size_t read_count = (size_t)(read_ratio * block_size);
    size_t write_count = block_size - read_count;

    // Warm-up to avoid cold-cache effects
    for (size_t warmup = 0; warmup < 5; warmup++) {
        for (size_t i = 0; i < total_size; i += block_size) {
            for (size_t j = 0; j < read_count; j++) {
                temp = array[i + j];  // Warm-up read operation
                (void)temp;  // Suppress unused variable warning
            }
            for (size_t j = 0; j < write_count; j++) {
                array[i + j] = (char)((i + j) & 0xFF);  // Warm-up write operation
            }
        }
    }

    // Start measuring cycles
    start = rdtsc_start();
    for (size_t iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < total_size; i += block_size) {
            // Perform reads
            for (size_t j = 0; j < read_count; j++) {
                temp = array[i + j];  // Read operation
                (void)temp;  // Suppress unused variable warning
            }

            // Perform writes
            for (size_t j = 0; j < write_count; j++) {
                array[i + j] = (char)((i + j) & 0xFF);  // Write operation
            }
        }
    }
    end = rdtsc_end();

    // Calculate the total time in cycles
    total_cycles = end - start;

    // Total data accessed in bytes
    double data_accessed = (double)((read_count + write_count) * iterations * (total_size / block_size));

    // Calculate bandwidth in bytes per second using the dynamic CPU frequency
    double bandwidth = data_accessed / (total_cycles / cpu_frequency);

    free(array); // Free the allocated memory
    return bandwidth;  // Bandwidth in bytes per second
}

void measure_maximum_bandwidth(size_t total_size) {
    size_t granularities[] = {64, 256, 1024};  // 64B, 256B, 1024B
    double ratios[] = {1.0, 0.0, 0.7, 0.5};    // Read: 100%, Write: 0%, 70:30, 50:50
    const char* ratio_labels[] = {"Read-only", "Write-only", "70:30 (R:W)", "50:50 (R:W)"};
    double cpu_frequency = get_cpu_frequency();
    // Print the CPU frequency
    printf("CPU Frequency: %.2f GHz\n", cpu_frequency / 1e9); // Convert Hz to GHz
    printf("Granularity\tRatio\t\tBandwidth (Gbps)\tRead Latency (Cycles)\tWrite Latency (Cycles)\n");
    printf("------------------------------------------------------------------------------------------------------------------\n");

    const size_t num_granularities = sizeof(granularities) / sizeof(granularities[0]);
    const size_t num_ratios = sizeof(ratios) / sizeof(ratios[0]);

    for (size_t i = 0; i < num_granularities; i++) {
        for (size_t j = 0; j < num_ratios; j++) {
            // Measure bandwidth
            double bandwidth_bytes_per_second = measure_bandwidth(granularities[i], ratios[j], total_size);
            if (bandwidth_bytes_per_second < 0) {
                fprintf(stderr, "Error measuring bandwidth for %zuB and ratio %.2f\n", granularities[i], ratios[j]);
                continue; // Skip to next iteration
            }
            double bandwidth_gbps = (bandwidth_bytes_per_second * 8) / 1e9;  // Convert to Gbps

            // Measure latencies
            double read_latency = measure_read_latency(granularities[i]);
            double write_latency = measure_write_latency(granularities[i]);

            // Print results in the specified format
            printf(" %zuB\t\t%s\t%.2f\t\t\t%.2f\t\t\t%.2f\n",
                   granularities[i], ratio_labels[j], bandwidth_gbps, read_latency, write_latency);
        }
    }
}

double measure_read_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;
    volatile char temp;

    // Memory barrier to avoid CPU reordering optimizations
    _mm_mfence();

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        temp = array[i];  // Read value
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    _mm_mfence();  // Memory barrier after the loop

    return (double)total_cycles / size;
}

double measure_write_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;

    // Memory barrier to avoid CPU reordering optimizations
    _mm_mfence();

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        array[i] = (char)(i & 0xFF);  // Write value
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    _mm_mfence();  // Memory barrier after the loop

    return (double)total_cycles / size;

}

double measure_bandwidth_with_queue(size_t block_size, double read_ratio, size_t total_size, size_t queue_depth) {
    uint64_t start, end;
    uint64_t total_cycles = 0;
    double cpu_frequency = get_cpu_frequency();  // Get actual CPU frequency
    size_t iterations = 100;  // Number of iterations for averaging
    volatile char temp;

    // Calculate read and write counts based on the specified ratio
    size_t read_count = (size_t)(read_ratio * block_size);
    size_t write_count = block_size - read_count;

    printf("Measuring bandwidth for block size: %zuB, read ratio: %.2f, queue depth: %zu\n", block_size, read_ratio, queue_depth);
    printf("Read count: %zu, Write count: %zu\n", read_count, write_count);

    // Simulate multiple outstanding requests by splitting work into 'queue_depth' parts
    printf("Starting warm-up phase...\n");
    for (size_t q = 0; q < queue_depth; q++) {
        for (size_t i = 0; i < total_size; i += block_size) {
            for (size_t j = 0; j < read_count; j++) {
                temp = array[i + j];  // Warm-up read operation
                (void)temp;
            }
            for (size_t j = 0; j < write_count; j++) {
                array[i + j] = (char)((i + j) & 0xFF);  // Warm-up write operation
            }
        }
    }

    // Start measuring cycles
    printf("Starting measurement phase...\n");
    start = rdtsc_start();
    for (size_t iter = 0; iter < iterations; iter++) {
        // Perform reads and writes in a loop
        for (size_t i = 0; i < total_size; i += block_size) {
            for (size_t j = 0; j < read_count; j++) {
                temp = array[i + j];  // Read operation
                (void)temp;
            }
            for (size_t j = 0; j < write_count; j++) {
                array[i + j] = (char)((i + j) & 0xFF);  // Write operation
            }
        }
    }
    end = rdtsc_end();
    total_cycles = end - start;

    // Calculate data accessed and bandwidth
    double data_accessed = (double)((read_count + write_count) * iterations * (total_size / block_size));
    double bandwidth = data_accessed / (total_cycles / cpu_frequency);
    double bandwidth_gbps = (bandwidth * 8) / 1e9;  // Convert bytes/s to Gbps

    printf("Total cycles: %lu, Data accessed: %.2f bytes\n", total_cycles, data_accessed);
    printf("Bandwidth: %.2f GBps (%.2f Gbps)\n", bandwidth / 1e9, bandwidth_gbps);

    return bandwidth;
}

void multiply_and_measure(size_t array_size) {
    // Allocate memory for the array
    double *array = malloc(array_size * sizeof(double));
    if (!array) {
        perror("Failed to allocate memory");
        return;
    }

    // Initialize the array with some values
    for (size_t i = 0; i < array_size; i++) {
        array[i] = (double)(i + 1);
    }

    // Initialize PAPI
    int retval = PAPI_library_init(PAPI_VERSION);
    if (retval != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error!\n");
        free(array);
        return;
    }

    // Declare variables to hold PAPI event values
    long long values[4]; // For cache misses and TLB misses
    int event_set = PAPI_NULL;

    // Create an event set
    PAPI_create_eventset(&event_set);

    // Add cache miss event (PAPI_L1_DCM for Level 1 Data Cache Miss)
    PAPI_add_event(event_set, PAPI_L1_DCM);
    // Add TLB miss events
    PAPI_add_event(event_set, PAPI_TLB_DM); // Data TLB misses
    PAPI_add_event(event_set, PAPI_TLB_IM); // Instruction TLB misses

    // Start counting events
    PAPI_start(event_set);

    // Start the timer
    clock_t start_time = clock();

    // Perform light computations (multiplications)
    double result = 1.0;
    for (size_t i = 0; i < array_size; i++) {
        result *= array[i];  // Multiply each element
    }

    // Stop counting events
    PAPI_stop(event_set, values);

    // Stop the timer
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    // Print the results
    printf("Array Size: %zu bytes, Execution Time: %.6f seconds, Result: %.2f, "
           "Cache Misses: %lld, Data TLB Misses: %lld, Instruction TLB Misses: %lld\n",
           array_size * sizeof(double), elapsed_time, result,
           values[0], values[1], values[2]);

    // Cleanup
    PAPI_cleanup_eventset(event_set);
    PAPI_destroy_eventset(&event_set);
    free(array);
}

size_t get_cache_size() {
    // Get cache size in bytes (L1 cache for example)
    long l1_cache_size = sysconf(_SC_LEVEL1_DCACHE_SIZE); // L1 data cache size
    return (size_t)l1_cache_size;
}

size_t get_memory_size() {
    // Get total system memory size in bytes
    long total_memory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE); // Total memory
    return (size_t)total_memory;
}


uint64_t measure_rdtsc_overhead() {
    uint64_t start, end;
    start = __rdtsc();
    end = __rdtsc();
    return end - start;  // Overhead in cycles
}


// Measure cache memory latency
double measure_cache_latency(size_t size, double cpu_freq) {
    volatile char *array = NULL;  // Declare array only once

    // Allocate aligned memory
    if (posix_memalign((void **)&array, 64, size) != 0) {
        perror("Failed to allocate memory");
        exit(1);
    }

    // Initialize the array
    for (size_t i = 0; i < size; i++) {
        array[i] = (char)(i & 0xFF);
    }

    uint64_t overhead = measure_rdtsc_overhead();

    // Measure latency
    uint64_t start, end;
    double total_time = 0.0;

    // Warm-up loop (linear access for cache)
    for (size_t i = 0; i < size; i++) {
        (void)array[i];
    }

    // Timing access
    for (size_t i = 0; i < size; i++) {
        start = __rdtsc();
        (void)array[i];
        end = __rdtsc();
        total_time += (end - start - overhead);  // Subtract overhead
    }

    free((void *)array);

    // Convert cycles to nanoseconds
    return (total_time / size) * (1e9 / cpu_freq);  // Average latency in ns
}



// Measure main memory latency
double measure_memory_latency(size_t size, double cpu_freq) {
    volatile char *array = malloc(size);
    if (array == NULL) {
        perror("Failed to allocate memory");
        return -1;
    }

    // Initialize the array
    for (size_t i = 0; i < size; i++) {
        array[i] = (char)(i & 0xFF);
    }

    uint64_t start, end;
    double total_time_ns = 0.0;

    // Warm-up loop
    for (size_t i = 0; i < size; i++) {
        (void)array[rand() % size]; // Random access to warm the memory
    }

    // Timing random access
    for (size_t i = 0; i < size; i++) {
        size_t index = rand() % size; // Random index
        start = __rdtsc();
        (void)array[index]; // Access to measure latency
        end = __rdtsc();
        total_time_ns += (end - start) * (1e9 / cpu_freq);
    }

    free((void *)array);
    return total_time_ns / size; // Return average latency in nanoseconds
}

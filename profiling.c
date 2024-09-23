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
    size_t iterations = 10;  // Number of iterations for averaging
    volatile char temp;

    // Calculate read and write counts based on the specified ratio
    size_t read_count = (size_t)(read_ratio * block_size);
    size_t write_count = block_size - read_count;

    // Warm-up to avoid cold-cache effects
    for (size_t warmup = 0; warmup < 5; warmup++) {
        for (size_t i = 0; i < total_size; i += block_size) {
            for (size_t j = 0; j < read_count; j++) {
                temp = array[i + j];  // Warm-up read operation
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
    double data_accessed = (double)(total_size * iterations);

    // Calculate bandwidth in bytes per second
    double bandwidth = data_accessed / (total_cycles / (double)CLOCKS_PER_SEC); // Convert cycles to seconds

    return bandwidth;  // Bandwidth in bytes per second
}

void measure_maximum_bandwidth(size_t total_size) {
    size_t granularities[] = {64, 256, 1024};  // 64B, 256B, 1024B
    double ratios[] = {1.0, 0.0, 0.7, 0.5};    // Read: 100%, Write: 0%, 70:30, 50:50
    const char* ratio_labels[] = {"Read-only", "Write-only", "70:30 (R:W)", "50:50 (R:W)"};

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

            // Measure latencies (implementations should be defined)
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

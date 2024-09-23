#define _GNU_SOURCE
#include "profiling.h"
#include "user_code.h"
#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

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

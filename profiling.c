#include "profiling.h"
#include "user_code.h"
#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>

volatile char *array;  // Global array, type matches declaration in profiling.h

uint64_t rdtsc_start() {
    return __rdtsc();
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
    for (size_t i = 0; i < size; i++) {
        array[i] = (char)(i & 0xFF);
    }
}

double measure_read_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        volatile char temp = array[i]; // Read value
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    return (double)total_cycles / size;
}


double measure_write_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        array[i] = (char)(i & 0xFF);
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    return (double)total_cycles / size;
}

#include "profiling.h"
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>

#define ARRAY_SIZE 32 * 1024 * 1024 // 32 MB

volatile char *array;

uint64_t rdtsc_start() {
    return __rdtsc(); // Read Time Stamp Counter
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
        array[i] = (char)(i & 0xFF); // Initialize array with some values
    }
}

double measure_read_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;
    volatile char temp;

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        temp = array[i];
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    return (double)total_cycles / size;
}

double measure_write_latency(size_t size) {
    uint64_t start, end, total_cycles = 0;

    for (size_t i = 0; i < size; i++) {
        start = rdtsc_start();
        array[i] = (char)(i & 0xFF); // Write values
        end = rdtsc_end();
        total_cycles += (end - start);
    }

    return (double)total_cycles / size;
}

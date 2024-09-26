#include <stdio.h>
#include <stdlib.h>
#include "profiling.h"

extern volatile char *array;  // Declare array as external

void profile_user_code(const char *user_program) {
    size_t size = 64 * 1024; // Example size: 64KB
    size_t main_mem = 16 * 1024 * 1024; // Example size: 16MB
    double cpu_freq = get_cpu_frequency();

    set_cpu_affinity(0);
    verify_cpu_affinity();

    // Initialize memory for profiling
    initialize_memory(size);

    // Measure latencies before execution
    double read_latency_before = measure_read_latency(size);
    double write_latency_before = measure_write_latency(size);

    size_t cache_size = get_cache_size();
    size_t memory_size = get_memory_size();
    double cache_latency = measure_cache_latency(size, cpu_freq);
    double memory_latency = measure_memory_latency(main_mem, cpu_freq);

    // Print system information
    printf("\n=== System Information ===\n");
    // printf("CPU Frequency: %.2f GHz\n", cpu_freq);
    printf("Cache Size: %zu bytes\n", cache_size);
    printf("Main Memory Size: %zu bytes\n", memory_size);

    // Print before execution results
    printf("\n=== Before Execution ===\n");
    printf("Read Latency: %.2f cycles\n", read_latency_before);
    printf("Write Latency: %.2f cycles\n", write_latency_before);
    printf("Cache Latency: %.2f ns\n", cache_latency);
    printf("Main Memory Latency: %.2f ns\n", memory_latency);

    // Execute user program
    printf("\n=== Executing User Program ===\n");
    printf("User Program: %s\n", user_program);

    char command[256];
    snprintf(command, sizeof(command), "%s", user_program);

    int result = system(command);
    if (result == -1) {
        perror("system");
        exit(1);
    } else {
        printf("User program executed successfully.\n");
    }

    // Measure latencies after execution
    double read_latency_after = measure_read_latency(size);
    double write_latency_after = measure_write_latency(size);

    // Print after execution results
    printf("\n=== After Execution ===\n");
    printf("Read Latency: %.2f cycles\n", read_latency_after);
    printf("Write Latency: %.2f cycles\n", write_latency_after);

    // Free allocated memory
    free((void*)array);
}

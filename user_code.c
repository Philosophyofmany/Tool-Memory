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

    // Call the bandwidth measurement function with different queue depths
    size_t block_size = 64;  // Example block size
    double read_ratio = 0.7; // Example read ratio
    size_t total_size = size; // Using the same size as the initial profiling

    printf("\n=== Measuring Bandwidth for Different Queue Depths ===\n");
    double previous_bandwidth = 0.0;
    size_t contention_queue_depth = 0;

    for (size_t queue_depth = 1; queue_depth <= 20; queue_depth *= 2) {
        printf("Testing with Queue Depth: %zu\n", queue_depth);
        double bandwidth = measure_bandwidth_with_queue(block_size, read_ratio, total_size, queue_depth);

        // Check for contention
        if (queue_depth > 1 && bandwidth < previous_bandwidth) {
            contention_queue_depth = queue_depth;
            printf("Contention detected at Queue Depth: %zu\n", queue_depth);
            break; // Exit loop after detecting contention
        }

        previous_bandwidth = bandwidth;
    }

    // If contention was never detected, output a message
    if (contention_queue_depth == 0) {
        printf("No contention detected in the tested queue depths.\n");
    }

    // Free allocated memory
    free((void*)array);
}

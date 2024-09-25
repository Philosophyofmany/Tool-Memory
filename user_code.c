#include <stdio.h>
#include <stdlib.h>
#include "profiling.h"

extern volatile char *array;  // Declare array as external

void profile_user_code(const char *user_program) {
    size_t size = 1024 * 1024 * 16; // Example size: 1MB

    set_cpu_affinity(0);
    verify_cpu_affinity();

    // Initialize memory for profiling
    initialize_memory(size);
    measure_bandwidth_with_queue(size, 0.7, size, 50);

    // Measure latencies before execution
    double read_latency_before = measure_read_latency(size);
    double write_latency_before = measure_write_latency(size);
    printf("Before execution:\n");
    printf("Read Latency: %.2f cycles\n", read_latency_before);
    printf("Write Latency: %.2f cycles\n", write_latency_before);

    // Execute user program
    printf("Executing user program: %s\n", user_program);
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
    printf("After execution:\n");
    printf("Read Latency: %.2f cycles\n", read_latency_after);
    printf("Write Latency: %.2f cycles\n", write_latency_after);

    // Free allocated memory
    free((void*)array);
}

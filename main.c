#include <stdio.h>
#include <stdlib.h>
#include "profiling.h"

int main(int argc, char **argv) {
    if (argc > 1) {
        profile_user_code(argv[1]);
    } else {
        size_t sizes[] = {1024, 1024 * 64, 1024 * 1024, 1024 * 1024 * 16}; // 1KB, 64KB, 1MB, 16MB
        size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

        set_cpu_affinity(0);
        verify_cpu_affinity();

        printf("Array Size (Bytes)\tRead Latency (Cycles)\tWrite Latency (Cycles)\n");
        printf("---------------------------------------------------------------\n");

        for (size_t i = 0; i < num_sizes; i++) {
            size_t size = sizes[i];
            initialize_memory(size);  // Allocate memory for the specific size

            double read_latency = measure_read_latency(size);
            double write_latency = measure_write_latency(size);

            // Print results
            printf("%-20lu\t%-20.2f\t%-20.2f\n", size, read_latency, write_latency);

            free((void*)array);  // Free the array after each test
            array = NULL;        // Avoid dangling pointer
        }
    }

    return 0;
}

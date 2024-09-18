#include <stdio.h>
#include <stdlib.h>
#include "profiling.h"
#include "user_code.h"

int main(int argc, char **argv) {
    if (argc > 1) {
        profile_user_code(argv[1]);
    } else {
        size_t sizes[] = {1024, 1024 * 64, 1024 * 1024, 1024 * 1024 * 16}; // 1KB, 64KB, 1MB, 16MB
        size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

        initialize_memory(ARRAY_SIZE);

        for (size_t i = 0; i < num_sizes; i++) {
            size_t size = sizes[i];
            printf("Testing array size: %lu bytes\n", size);
            double read_latency = measure_read_latency(size);
            double write_latency = measure_write_latency(size);
            printf("Read Latency: %.2f cycles\n", read_latency);
            printf("Write Latency: %.2f cycles\n", write_latency);
        }

        free((void*)array);
    }

    return 0;
}

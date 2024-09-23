#ifndef PROFILING_H
#define PROFILING_H

#include <stdint.h>
#include <stdlib.h>

extern volatile char *array;  // Global array

#define ARRAY_SIZE (32 * 1024 * 1024)  // 32MB

void initialize_memory(size_t size);
double measure_read_latency(size_t size);
double measure_write_latency(size_t size);
void set_cpu_affinity(int core_id);
void verify_cpu_affinity();
double measure_bandwidth(size_t block_size, double read_ratio, size_t total_size);
void measure_maximum_bandwidth(size_t total_size);

// Consistent declaration with user_code.c
void profile_user_code(const char *arg);

#endif // PROFILING_H

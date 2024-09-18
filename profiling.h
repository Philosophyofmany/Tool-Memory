#ifndef PROFILING_H
#define PROFILING_H

#include <stdint.h>

void initialize_memory(size_t size);
double measure_read_latency(size_t size);
double measure_write_latency(size_t size);

#endif // PROFILING_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

void* grid_platform_allocate_volatile(size_t size) { return malloc(size); }

uint64_t grid_platform_rtc_get_micros(void) { return 0; }

uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return 0; }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return 10 - told; }

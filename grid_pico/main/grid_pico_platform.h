#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pico/stdlib.h"

uint64_t grid_platform_rtc_get_micros(void);
uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);
void* grid_platform_allocate_volatile(size_t size);
void grid_platform_printf(char const* fmt, ...);

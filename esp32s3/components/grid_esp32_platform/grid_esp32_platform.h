#ifndef GRID_ESP32_PLATFORM_H
#define GRID_ESP32_PLATFORM_H

#include "esp_attr.h"
#include <stddef.h>
#include <stdint.h>

void* grid_platform_allocate_volatile(size_t size);

uint64_t grid_platform_rtc_get_micros(void);

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

uint32_t grid_platform_get_cycles();

uint32_t grid_platform_get_cycles_per_us();

#endif /* GRID_ESP32_PLATFORM_H */

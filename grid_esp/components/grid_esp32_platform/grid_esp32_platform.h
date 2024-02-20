

#pragma once

#include "esp_attr.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRID_ESP32_PLATFORM_FEATURE_NO_RXDOUBLEBUFFER_ON_UART 1

void* grid_platform_allocate_volatile(size_t size);

uint64_t IRAM_ATTR grid_platform_rtc_get_micros(void);

uint64_t IRAM_ATTR grid_platform_rtc_get_elapsed_time(uint64_t told);

uint32_t IRAM_ATTR grid_platform_get_cycles();

uint32_t IRAM_ATTR grid_platform_get_cycles_per_us();

#ifdef __cplusplus
}
#endif



#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRID_ESP32_PLATFORM_FEATURE_NO_RXDOUBLEBUFFER_ON_UART 0

void* grid_platform_allocate_volatile(size_t size);

#ifdef __cplusplus
}
#endif

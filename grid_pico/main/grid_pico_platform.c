#include "grid_pico_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// uint64_t grid_platform_rtc_get_micros(void) { return time_us_64(); }

// uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return time_us_64() - told; }

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = malloc(size);
  if (handle == NULL) {

    printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}

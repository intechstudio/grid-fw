/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_platform.h"

#include "esp_heap_caps.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_timer.h"

// #include "hal/cpu_hal.h"

#include "esp_cpu.h"

// static const char* TAG = "grid_esp32_platform";

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // ets_printf("ADDRESS: %lx\r\n", handle);

  if (handle == NULL) {

    ets_printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}

uint64_t IRAM_ATTR grid_platform_rtc_get_micros(void) { return esp_timer_get_time(); }

uint64_t IRAM_ATTR grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_micros() - told; }

uint32_t IRAM_ATTR grid_platform_get_cycles() { return esp_cpu_get_cycle_count(); }

uint32_t IRAM_ATTR grid_platform_get_cycles_per_us() { return 240; }

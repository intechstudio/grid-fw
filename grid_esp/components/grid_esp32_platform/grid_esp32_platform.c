/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_platform.h"

#include "driver/gpio.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "driver/ledc.h" // for pwm based pico system clock generation
#include "rom/ets_sys.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf

#include "esp_random.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "hal/cpu_hal.h"

static const char* TAG = "grid_esp32_platform";

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

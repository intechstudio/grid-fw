/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_pbf4.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_esp32_adc.h"

static const char* TAG = "module_pbf4";

void grid_esp32_module_pbf4_task(void* arg) {

  uint64_t potmeter_last_real_time[16] = {0};
  static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, -1, -1, -1, -1, 10, 8, 11, 9};

  static const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state, (SemaphoreHandle_t)arg);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  while (1) {

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 10);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;

      if (invert_result_lookup[lookup_index]) {
        result->value = 4095 - result->value;
      }

      if (multiplexer_lookup[lookup_index] < 8) {

        grid_ui_potmeter_store_input(multiplexer_lookup[lookup_index], &potmeter_last_real_time[lookup_index], result->value, 12);
      } else if (multiplexer_lookup[lookup_index] < 12) {

        grid_ui_button_store_input(multiplexer_lookup[lookup_index], &potmeter_last_real_time[lookup_index], result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

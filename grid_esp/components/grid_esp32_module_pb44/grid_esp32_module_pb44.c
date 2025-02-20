/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_pb44.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

static const char* TAG = "module_pb44";

#define GRID_MODULE_PB44_BUT_NUM 8

void grid_esp32_module_pb44_task(void* arg) {

  static struct grid_ui_button_state ui_button_state[GRID_MODULE_PB44_BUT_NUM] = {0};

  for (int i = 0; i < GRID_MODULE_PB44_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  uint64_t potmeter_last_real_time[16] = {0};
  static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};

  static const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  while (1) {

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 10);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (invert_result_lookup[lookup_index]) {
        result->value = 4095 - result->value;
      }

      if (mux_position < 8) {

        grid_ui_potmeter_store_input(mux_position, &potmeter_last_real_time[lookup_index], result->value, 12);
      } else if (mux_position < 16) {

        grid_ui_button_store_input(&ui_button_state[mux_position - 8], mux_position, result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

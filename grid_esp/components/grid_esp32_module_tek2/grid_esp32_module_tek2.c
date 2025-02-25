/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_tek2.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_endless.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

static const char* TAG = "module_tek2";

#define GRID_MODULE_TEK2_POT_NUM 2

void grid_esp32_module_tek2_task(void* arg) {

  uint64_t button_last_real_time[8] = {0};

  // static const uint8_t multiplexer_lookup[16] = {10, 8, 11, 9, 14, 12, 15,
  // 13, 2, 0, 3, 1, 6, 4, 7, 5};
  static const uint8_t multiplexer_lookup[16] = {9, 8, 11, 10, 13, 12, -1, -1, 2, 0, 3, 1, 6, 4, 7, 5};

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  struct grid_ui_endless_state new_endless_state[GRID_MODULE_TEK2_POT_NUM] = {0};
  struct grid_ui_endless_state old_endless_state[GRID_MODULE_TEK2_POT_NUM] = {0};

  while (1) {

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;

      if (multiplexer_lookup[lookup_index] < 8) {

        grid_ui_button_store_input(multiplexer_lookup[lookup_index], &button_last_real_time[multiplexer_lookup[lookup_index]], result->value, 12);
      } else if (multiplexer_lookup[lookup_index] < 10) { // 8, 9

        uint8_t endless_index = multiplexer_lookup[lookup_index] % 2;
        new_endless_state[endless_index].phase_a = result->value;
      } else if (multiplexer_lookup[lookup_index] < 12) { // 10, 11

        uint8_t endless_index = multiplexer_lookup[lookup_index] % 2;
        new_endless_state[endless_index].phase_b = result->value;
      } else if (multiplexer_lookup[lookup_index] < 14) { // 12, 13

        uint8_t endless_index = multiplexer_lookup[lookup_index] % 2;
        new_endless_state[endless_index].button_value = result->value;
        grid_ui_button_store_input(8 + endless_index, &old_endless_state[endless_index].button_last_real_time, result->value, 12);

        // grid_ui_endless_store_input(8 + endless_index, 12, &new_endless_state[endless_index], &old_endless_state[endless_index]);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

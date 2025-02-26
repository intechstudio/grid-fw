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

#define GRID_MODULE_TEK2_BUT_NUM 10

static struct grid_ui_element* DRAM_ATTR elements = NULL;

void grid_esp32_module_tek2_task(void* arg) {

  static struct grid_ui_button_state ui_button_state[GRID_MODULE_TEK2_BUT_NUM] = {0};

  for (int i = 0; i < GRID_MODULE_TEK2_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  // static const uint8_t multiplexer_lookup[16] = {10, 8, 11, 9, 14, 12, 15,
  // 13, 2, 0, 3, 1, 6, 4, 7, 5};
  static const uint8_t multiplexer_lookup[16] = {9, 8, 11, 10, 13, 12, -1, -1, 2, 0, 3, 1, 6, 4, 7, 5};

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  struct grid_ui_endless_state new_endless_state[GRID_MODULE_TEK2_POT_NUM] = {0};
  struct grid_ui_endless_state old_endless_state[GRID_MODULE_TEK2_POT_NUM] = {0};

  while (1) {

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];
      struct grid_ui_element* ele = &elements[mux_position];

      if (mux_position < 8) {

        grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
      } else if (mux_position < 10) { // 8, 9

        uint8_t endless_index = mux_position % 2;
        new_endless_state[endless_index].phase_a = result->value;
      } else if (mux_position < 12) { // 10, 11

        uint8_t endless_index = mux_position % 2;
        new_endless_state[endless_index].phase_b = result->value;
      } else if (mux_position < 14) { // 12, 13

        uint8_t endless_index = mux_position % 2;
        new_endless_state[endless_index].button_value = result->value;
        grid_ui_button_store_input(ele, &ui_button_state[8 + endless_index], result->value, 12);

        // grid_ui_endless_store_input(8 + endless_index, 12, &new_endless_state[endless_index], &old_endless_state[endless_index]);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

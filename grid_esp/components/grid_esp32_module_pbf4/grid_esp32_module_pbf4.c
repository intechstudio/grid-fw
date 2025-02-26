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

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

static const char* TAG = "module_pbf4";

#define GRID_MODULE_PBF4_BUT_NUM 4

void grid_esp32_module_pbf4_task(void* arg) {

  static struct grid_ui_button_state ui_button_state[GRID_MODULE_PBF4_BUT_NUM] = {0};

  for (int i = 0; i < GRID_MODULE_PBF4_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  uint64_t potmeter_last_real_time[16] = {0};
  static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, -1, -1, -1, -1, 10, 8, 11, 9};

  static const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_cal_init(&grid_cal_state, 12, grid_ui_state.element_list_length);
  grid_cal_enable_range(&grid_cal_state, 0, 4);

  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL);
  while (grid_ui_state.bulk_status == GRID_UI_BULK_CONFREAD_PROGRESS) {
    taskYIELD();
  }

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

        uint16_t calibrated;
        grid_cal_next(&grid_cal_state, mux_position, result->value, &calibrated);
        grid_ui_potmeter_store_input(mux_position, &potmeter_last_real_time[lookup_index], calibrated, 12);
      } else if (mux_position < 12) {

        grid_ui_button_store_input(&ui_button_state[mux_position - 8], mux_position, result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

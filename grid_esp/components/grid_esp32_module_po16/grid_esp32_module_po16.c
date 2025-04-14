/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_po16.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_po16";

#define GRID_MODULE_PO16_POT_NUM 16

static uint64_t* DRAM_ATTR potmeter_last_real_time = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR po16_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};
  static DRAM_ATTR const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (invert_result_lookup[lookup_index]) {
    result->value = 4095 - result->value;
  }

  if (mux_position < 16) {

    uint16_t calibrated;
    grid_cal_next(&grid_cal_state, mux_position, result->value, &calibrated);
    grid_ui_potmeter_store_input(ele, mux_position, &potmeter_last_real_time[mux_position], calibrated, 12);
  }
}

void grid_esp32_module_po16_task(void* arg) {

  potmeter_last_real_time = grid_platform_allocate_volatile(GRID_MODULE_PO16_POT_NUM * sizeof(uint64_t));
  memset(potmeter_last_real_time, 0, GRID_MODULE_PO16_POT_NUM * sizeof(uint64_t));

  grid_cal_init(&grid_cal_state, 12, grid_ui_state.element_list_length);
  grid_cal_enable_range(&grid_cal_state, 0, 16);

  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL);
  while (grid_ui_state.bulk_status == GRID_UI_BULK_CONFREAD_PROGRESS) {
    taskYIELD();
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, po16_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, 8);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

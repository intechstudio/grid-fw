/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_bu16.h"

#include "grid_ui_button.h"
#include "grid_ui_system.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_bu16";

#define GRID_MODULE_BU16_BUT_NUM 16

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR bu16_process_analog(void* user) {

  static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (mux_position < 16) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
  }
}

void grid_esp32_module_bu16_task(void* arg) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_BU16_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_button_state, 0, GRID_MODULE_BU16_BUT_NUM * sizeof(struct grid_ui_button_state));

  for (int i = 0; i < GRID_MODULE_BU16_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, bu16_process_analog);

  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_start(&grid_esp32_adc_state, multiplexer_overflow, grid_hwcfg_module_is_rev_h(&grid_sys_state));

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_pb44.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_pb44";

#define GRID_MODULE_PB44_BUT_NUM 8

#define GRID_MODULE_PB44_POT_NUM 8

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static uint64_t* DRAM_ATTR potmeter_last_real_time = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR pb44_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};
  static DRAM_ATTR const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (invert_result_lookup[lookup_index]) {
    result->value = 4095 - result->value;
  }

  if (mux_position < 8) {

    grid_ui_potmeter_store_input(ele, mux_position, &potmeter_last_real_time[mux_position], result->value, 12);
  } else if (mux_position < 16) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position - 8], result->value, 12);
  }
}

void grid_esp32_module_pb44_task(void* arg) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_PB44_BUT_NUM * sizeof(struct grid_ui_button_state));
  potmeter_last_real_time = grid_platform_allocate_volatile(GRID_MODULE_PB44_POT_NUM * sizeof(uint64_t));
  memset(ui_button_state, 0, GRID_MODULE_PB44_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(potmeter_last_real_time, 0, GRID_MODULE_PB44_POT_NUM * sizeof(uint64_t));

  for (int i = 0; i < GRID_MODULE_PB44_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, pb44_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, 8);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(&grid_sys_state);
  grid_esp32_adc_start(&grid_esp32_adc_state, mux_dependent);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  GRID_MODULE_DRIVER_INIT_DONE = 1;

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

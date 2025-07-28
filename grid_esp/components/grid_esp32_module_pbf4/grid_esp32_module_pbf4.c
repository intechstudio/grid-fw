/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_pbf4.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_module.h"
#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_pbf4";

#define GRID_MODULE_PBF4_BUT_NUM 4

#define GRID_MODULE_PBF4_POT_NUM 8

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static uint64_t* DRAM_ATTR potmeter_last_real_time = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR pbf4_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, -1, -1, -1, -1, 10, 8, 11, 9};
  static DRAM_ATTR const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (invert_result_lookup[lookup_index]) {
    result->value = 4095 - result->value;
  }

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 8) {

    uint16_t calibrated;
    grid_cal_pot_next(&grid_cal_state.potmeter, mux_position, result->value, &calibrated);
    grid_ui_potmeter_store_input(ele, mux_position, &potmeter_last_real_time[mux_position], calibrated, 12);
  } else if (mux_position < 12) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position - 8], result->value, 12);
  }
}

void grid_esp32_module_pbf4_task(void* arg) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_PBF4_BUT_NUM * sizeof(struct grid_ui_button_state));
  potmeter_last_real_time = grid_platform_allocate_volatile(GRID_MODULE_PBF4_POT_NUM * sizeof(uint64_t));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_PBF4_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(potmeter_last_real_time, 0, GRID_MODULE_PBF4_POT_NUM * sizeof(uint64_t));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_PBF4_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);
  if (grid_hwcfg_module_is_rev_h(&grid_sys_state)) {
    grid_asc_array_set_factors(asc_state, 16, 12, 4, 1);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  grid_config_init(&grid_config_state, &grid_cal_state);

  struct grid_cal_pot* cal_pot = &grid_cal_state.potmeter;
  grid_cal_pot_init(cal_pot, 12, grid_ui_state.element_list_length);
  grid_cal_pot_enable_range(cal_pot, 0, 4);

  if (grid_hwcfg_module_is_rev_h(&grid_sys_state)) {

    struct grid_cal_but* cal_but = &grid_cal_state.button;
    grid_cal_but_init(cal_but, grid_ui_state.element_list_length);
    for (int i = 8; i < 12; ++i) {
      grid_cal_but_enable_set(cal_but, i, &ui_button_state[i - 8]);
    }
  }

  while (grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    taskYIELD();
  }

  while (grid_ui_bulk_is_in_progress(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    taskYIELD();
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, pbf4_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, 8);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(&grid_sys_state);
  grid_esp32_adc_start(&grid_esp32_adc_state, mux_dependent);

  GRID_MODULE_DRIVER_INIT_DONE = 1;

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

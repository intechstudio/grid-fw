/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_pbf4.h"

#include <stdint.h>

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
static struct grid_ui_potmeter_state* DRAM_ATTR ui_potmeter_state = NULL;
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

    grid_ui_potmeter_store_input(ele, mux_position, &ui_potmeter_state[mux_position], result->value, 12);

  } else if (mux_position < 12) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position - 8], result->value, 12);
  }
}

void grid_esp32_module_pbf4_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_PBF4_BUT_NUM * sizeof(struct grid_ui_button_state));
  ui_potmeter_state = grid_platform_allocate_volatile(GRID_MODULE_PBF4_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_PBF4_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_potmeter_state, 0, GRID_MODULE_PBF4_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_PBF4_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  for (int i = 0; i < GRID_MODULE_PBF4_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], 12, 64, 2192);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);
  if (grid_hwcfg_module_is_rev_h(sys)) {
    grid_asc_array_set_factors(asc_state, 16, 12, 4, 1);
  }

  elements = grid_ui_model_get_elements(ui);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, 12);

  for (int i = 0; i < 4; ++i) {
    assert(grid_cal_set(cal, i, GRID_CAL_CENTER, &ui_potmeter_state[i].center) == 0);
    assert(grid_cal_set(cal, i, GRID_CAL_DETENT, &ui_potmeter_state[i].detent) == 0);
  }

  for (int i = 0; i < 8; ++i) {
    assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &ui_potmeter_state[i].limits) == 0);
  }

  if (grid_hwcfg_module_is_rev_h(sys)) {
    for (int i = 8; i < 12; ++i) {
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &ui_button_state[i - 8].limits) == 0);
    }
  }

  while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    vTaskDelay(1);
  }

  while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    vTaskDelay(1);
  }

  grid_esp32_adc_init(adc, pbf4_process_analog);
  grid_esp32_adc_mux_init(adc, 8);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

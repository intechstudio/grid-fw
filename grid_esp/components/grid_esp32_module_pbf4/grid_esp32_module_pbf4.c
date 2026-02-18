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
#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_pbf4";

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;

#define X GRID_MUX_UNUSED
static DRAM_ATTR const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, X, X, 8, 9},
    {2, 3, 6, 7, X, X, 10, 11},
};
#undef X
static DRAM_ATTR uint16_t element_invert_bm = 0b0000000000001111;

void IRAM_ATTR pbf4_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  assert(element_index != GRID_MUX_UNUSED);

  uint16_t raw = result->value;
  uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);

  uint16_t processed;
  if (!grid_asc_process(asc_state, element_index, inverted, &processed)) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
    grid_ui_potmeter_store_input(ele, processed);
  } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
    struct grid_ui_button_state* bstate = (struct grid_ui_button_state*)ele->primary_state;
    grid_ui_button_store_input(ele, bstate, processed);
  }
}

void grid_esp32_module_pbf4_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
      grid_ui_potmeter_configure(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      grid_ui_button_configure(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
    }
  }

  asc_state = grid_platform_allocate_volatile(12 * sizeof(struct grid_asc));
  memset(asc_state, 0, 12 * sizeof(struct grid_asc));

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, 12);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER && i < 4) {
      struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
      grid_asc_set_factor(asc_state, i, 8);
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
      assert(grid_cal_set(cal, i, GRID_CAL_CENTER, &state->center) == 0);
      assert(grid_cal_set(cal, i, GRID_CAL_DETENT, &state->detent) == 0);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
      grid_asc_set_factor(asc_state, i, 8);
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      grid_asc_set_factor(asc_state, i, grid_hwcfg_module_is_rev_h(sys) ? 1 : 8);
      if (grid_hwcfg_module_is_rev_h(sys)) {
        assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
      }
    }
  }

  while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    vTaskDelay(1);
  }

  while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    vTaskDelay(1);
  }

  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_init(adc, 0b11001111, mux_dependent, pbf4_process_analog);
  grid_esp32_adc_start(adc);
}

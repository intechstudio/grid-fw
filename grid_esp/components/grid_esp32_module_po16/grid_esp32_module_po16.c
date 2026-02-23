/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_po16.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_ui.h"

#include "grid_ui_potmeter.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_po16";

#define GRID_MODULE_PO16_ASC_FACTOR 8

static DRAM_ATTR const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},
    {2, 3, 6, 7, 10, 11, 14, 15},
};
static DRAM_ATTR uint16_t element_invert_bm = 0b1111111111111111;

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;
static struct grid_asc* DRAM_ATTR asc_array = NULL;
static uint8_t DRAM_ATTR asc_array_length = 0;

void IRAM_ATTR po16_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  result->value = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  assert(element_index < asc_array_length);
  if (!grid_asc_process(&asc_array[element_index], result->value, &result->value)) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), result->value);
}

void grid_esp32_module_po16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  asc_array_length = ui->element_list_length - 1;
  asc_array = grid_platform_allocate_volatile(asc_array_length * sizeof(struct grid_asc));
  memset(asc_array, 0, asc_array_length * sizeof(struct grid_asc));

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = grid_ui_potmeter_get_state(ele);
      grid_ui_potmeter_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
      grid_asc_set_factor(&asc_array[i], GRID_MODULE_PO16_ASC_FACTOR);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &state->limits);
      grid_cal_channel_set(cal, i, GRID_CAL_CENTER, &state->center);
      grid_cal_channel_set(cal, i, GRID_CAL_DETENT, &state->detent);
    }
  }

  while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    vTaskDelay(1);
  }

  while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    vTaskDelay(1);
  }

  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_init(adc, 0b11111111, mux_dependent, po16_process_analog);
  grid_esp32_adc_start(adc);
}

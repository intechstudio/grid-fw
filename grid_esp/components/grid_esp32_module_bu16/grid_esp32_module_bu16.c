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
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_module.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_bu16";

#define GRID_MODULE_BU16_BUTTON_COUNT 16

static struct grid_asc* DRAM_ATTR asc_state = NULL;

static DRAM_ATTR const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},
    {2, 3, 6, 7, 10, 11, 14, 15},
};
static DRAM_ATTR uint16_t element_invert_bm = 0;

void IRAM_ATTR bu16_process_analog(void* user) {

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  uint16_t raw = result->value;
  uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);
  uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

  uint16_t processed;
  if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
    return;
  }

  grid_ui_button_store_input(&grid_ui_state, element_index, processed);
}

void grid_esp32_module_bu16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_BU16_BUTTON_COUNT; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
    grid_ui_button_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 1);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  if (grid_hwcfg_module_is_rev_h(sys)) {

    for (int i = 0; i < GRID_MODULE_BU16_BUTTON_COUNT; ++i) {
      struct grid_ui_element* ele = &ui->element_list[i];
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
    }

    assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
    grid_ui_bulk_flush(ui);
  }

  grid_esp32_adc_init(adc, bu16_process_analog);
  grid_platform_mux_init(0b11111111);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

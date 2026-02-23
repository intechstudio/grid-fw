/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_ef44.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_ef44";

#define GRID_MODULE_EF44_ENCODER_COUNT 4
#define GRID_MODULE_EF44_ASC_FACTOR 16

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;

static DRAM_ATTR const uint8_t mux_element_lookup[2][2] = {
    {4, 5},
    {6, 7},
};
static DRAM_ATTR uint16_t element_invert_bm = 0;

void IRAM_ATTR ef44_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  uint16_t raw = result->value;
  uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);

  uint16_t processed;
  if (!grid_asc_process(&asc_state[element_index], inverted, &processed)) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), processed);
}

void IRAM_ATTR ef44_process_encoder(struct grid_encoder_result* result) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EF44_ENCODER_COUNT] = {2, 3, 0, 1};

  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENCODER_COUNT; ++i) {

    assert(i / 2 < result->length);
    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, i);
    uint8_t element_index = encoder_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_encoder_store_input(grid_ui_encoder_get_state(ele), sample);
  }
}

void grid_esp32_module_ef44_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_esp32_encoder_model* enc, struct grid_config_model* conf,
                                 struct grid_cal_model* cal) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

  asc_state = grid_platform_allocate_volatile(8 * sizeof(struct grid_asc));
  memset(asc_state, 0, 8 * sizeof(struct grid_asc));

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = grid_ui_potmeter_get_state(ele);
      grid_ui_potmeter_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
      grid_asc_set_factor(&asc_state[i], GRID_MODULE_EF44_ASC_FACTOR);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &state->limits);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      grid_ui_encoder_state_init(grid_ui_encoder_get_state(ele), detent, direction);
    }
  }

  assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(ui);

  uint8_t transfer_length = 1 + GRID_MODULE_EF44_ENCODER_COUNT / 2;
  // I2S clock rate chosen so callback fires at 500 Hz: rate = 500 * 32bit * 4slots
  uint32_t clock_rate = 500 * I2S_DATA_BIT_WIDTH_32BIT * 4;
  grid_esp32_encoder_init(enc, transfer_length, clock_rate, ef44_process_encoder);

  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_init(adc, 0b00000011, mux_dependent, ef44_process_analog);
  grid_esp32_encoder_start(enc);
  grid_esp32_adc_start(adc);
}

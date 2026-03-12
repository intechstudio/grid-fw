/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_octv.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_cal.h"
#include "grid_config.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_octv";

#define GRID_MODULE_OCTV_ENC_NUM 8

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;

#define X GRID_MUX_UNUSED
static DRAM_ATTR const uint8_t mux_element_lookup[2][8] = {
    {8, 9, 10, 11, 12, 13, 14, X},
    {15, 16, 17, 18, 19, 20, X, X},
};
#undef X

void IRAM_ATTR octv_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  if (element_index == GRID_MUX_UNUSED) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_button_store_input(grid_ui_button_get_state(ele), result->value);
}

void IRAM_ATTR octv_process_encoder(struct grid_encoder_result* result) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_OCTV_ENC_NUM] = {6, 7, 4, 5, 2, 3, 0, 1};

  for (uint8_t j = 0; j < GRID_MODULE_OCTV_ENC_NUM; ++j) {

    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, j);
    uint8_t idx = encoder_lookup[j];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[idx];
    grid_ui_encoder_store_input(grid_ui_encoder_get_state(ele), sample);
  }
}

void grid_esp32_module_octv_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_esp32_encoder_model* enc, struct grid_config_model* conf,
                                 struct grid_cal_model* cal) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      grid_ui_encoder_state_init(grid_ui_encoder_get_state(ele), detent, direction);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      grid_ui_button_state_init(grid_ui_button_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &grid_ui_button_get_state(ele)->limits);
    }
  }

  assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(ui);

  uint8_t transfer_length = 1 + GRID_MODULE_OCTV_ENC_NUM / 2;
  // I2S clock rate chosen so callback fires at 500 Hz: rate = 500 * 32bit * 4slots
  uint32_t clock_rate = 500 * I2S_DATA_BIT_WIDTH_32BIT * 4;
  grid_esp32_encoder_init(enc, transfer_length, clock_rate, octv_process_encoder);

  grid_esp32_adc_init(adc, 0b01111111, false, octv_process_analog);
  grid_esp32_encoder_start(enc);
  grid_esp32_adc_start(adc);
}

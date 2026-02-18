/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_en16.h"

#include <stdint.h>

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_encoder.h"

// static const char* TAG = "module_en16";

#define GRID_MODULE_EN16_ENCODER_COUNT 16

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;

void IRAM_ATTR en16_process_encoder(struct grid_encoder_result* result) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EN16_ENCODER_COUNT] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENCODER_COUNT; ++i) {

    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, i);
    uint8_t element_index = encoder_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_encoder_store_input(grid_ui_encoder_get_state(ele), sample);
  }
}

void grid_esp32_module_en16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_encoder_model* enc) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      grid_ui_encoder_configure(grid_ui_encoder_get_state(ele), detent, direction);
    }
  }

  uint8_t transfer_length = 1 + GRID_MODULE_EN16_ENCODER_COUNT / 2;
  // I2S clock rate chosen so callback fires at 1000 Hz: rate = 1000 * 32bit * 4slots
  uint32_t clock_rate = 1000 * I2S_DATA_BIT_WIDTH_32BIT * 4;
  grid_esp32_encoder_init(enc, transfer_length, clock_rate, en16_process_encoder);
}

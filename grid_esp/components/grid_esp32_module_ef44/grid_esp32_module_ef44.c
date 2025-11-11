/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_ef44.h"

#include <stdint.h>

#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_ef44";

#define GRID_MODULE_EF44_BUT_NUM 4

#define GRID_MODULE_EF44_ENC_NUM 4

#define GRID_MODULE_EF44_POT_NUM 4

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_encoder_state* DRAM_ATTR ui_encoder_state = NULL;
static struct grid_ui_potmeter_state* DRAM_ATTR ui_potmeter_state = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR ef44_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {6, 4, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, 10, 8, 11, 9};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 4) {

  } else if (mux_position < 8) {

    grid_ui_potmeter_store_input(ele, mux_position, &ui_potmeter_state[mux_position - 4], result->value, 12);
  }
}

void IRAM_ATTR ef44_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EF44_ENC_NUM] = {2, 3, 0, 1};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t j = 0; j < GRID_MODULE_EF44_ENC_NUM; ++j) {

    uint8_t value = (bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);

    uint8_t button_value = value & 0b00000100;

    grid_ui_button_store_input(ele, &ui_button_state[idx], button_value, 1);
  }
}

void grid_esp32_module_ef44_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_esp32_encoder_model* enc, struct grid_config_model* conf,
                                 struct grid_cal_model* cal) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_BUT_NUM * sizeof(struct grid_ui_button_state));
  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  ui_potmeter_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_EF44_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_encoder_state, 0, GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(ui_potmeter_state, 0, GRID_MODULE_EF44_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_EF44_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  for (int i = 0; i < GRID_MODULE_EF44_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], 12, 64, 2192);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);

  elements = grid_ui_model_get_elements(ui);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, 12);

  for (int i = 4; i < 8; ++i) {
    assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &ui_potmeter_state[i - 4].limits) == 0);
  }

  while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    vTaskDelay(1);
  }

  while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    vTaskDelay(1);
  }

  grid_esp32_encoder_init(enc, 1, ef44_process_encoder);
  uint8_t detent = grid_sys_get_hwcfg(sys) != GRID_MODULE_EF44_ND_RevD;
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  grid_esp32_adc_init(adc, ef44_process_analog);
  grid_esp32_adc_mux_init(adc, 2);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

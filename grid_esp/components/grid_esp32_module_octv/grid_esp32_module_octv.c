/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_octv.h"

#include <stdint.h>

#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_system.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_octv";

// 8 encoder push buttons + 16 analog buttons = 24 button states
#define GRID_MODULE_OCTV_BUT_NUM 24

// 8 rotary encoders
#define GRID_MODULE_OCTV_ENC_NUM 8

// 16 pressure-sensitive buttons via ADC
#define GRID_MODULE_OCTV_ANALOG_BUT_NUM 16

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_encoder_state* DRAM_ATTR ui_encoder_state = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

/*
 * ADC callback for processing pressure-sensitive buttons.
 * Maps ADC channels via multiplexer to UI button elements 8-23.
 * Uses the same multiplexer mapping as BU16, offset by 8 for element indices.
 */
void IRAM_ATTR octv_process_analog(void* user) {

  // OCTV multiplexer lookup for button elements 8-23
  // Adjusted based on hardware testing
  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {15, 8, 16, 9, 17, 10, 18, 11, 19, 12, 20, 13, 22, 14, 23, 21};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t element_index = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[element_index];

  // Apply auto-scale correction filtering
  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  // Store input for analog buttons (elements 8-23)
  grid_ui_button_store_input(ele, &ui_button_state[element_index], result->value, 12);
}

/*
 * Encoder callback for processing rotary encoders via I2S TDM.
 * Maps encoder data to UI encoder elements 0-7.
 * Each encoder provides rotation + push button state.
 */
void IRAM_ATTR octv_process_encoder(void* dma_buf) {

  // Encoder lookup table: maps physical encoder position to logical element index
  // Based on EN16's 2-by-2 swap pattern for the first 8 positions
  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_OCTV_ENC_NUM] = {6, 7, 4, 5, 2, 3, 0, 1};

  // Skip hwcfg byte at start of DMA buffer
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t j = 0; j < GRID_MODULE_OCTV_ENC_NUM; ++j) {

    // Extract 4-bit encoder value (2 encoders per byte)
    uint8_t value = (bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    // Process encoder rotation
    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);

    // Extract button state from encoder nibble (bit 2 indicates button)
    uint8_t button_value = value & 0b00000100;

    // Process encoder push button (elements 0-7 have integrated buttons)
    grid_ui_button_store_input(ele, &ui_button_state[idx], button_value, 1);
  }
}

void grid_esp32_module_octv_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_esp32_encoder_model* enc, struct grid_config_model* conf,
                                 struct grid_cal_model* cal) {

  // Allocate state arrays in volatile/DMA-capable memory
  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_OCTV_BUT_NUM * sizeof(struct grid_ui_button_state));
  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_OCTV_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc)); // 16 ADC channels

  memset(ui_button_state, 0, GRID_MODULE_OCTV_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_encoder_state, 0, GRID_MODULE_OCTV_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  // Initialize encoder push button states (elements 0-7)
  // These are digital buttons, so use 1-bit depth
  for (int i = 0; i < GRID_MODULE_OCTV_ENC_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  // Initialize analog button states (elements 8-23)
  // These are 12-bit ADC with pressure sensitivity
  for (int i = GRID_MODULE_OCTV_ENC_NUM; i < GRID_MODULE_OCTV_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  // Set up auto-scale correction factors for ADC channels
  // Factor of 1 means minimal averaging (responsive)
  grid_asc_array_set_factors(asc_state, 16, 0, 16, 1);

  // Get pointer to UI elements for use in ISR callbacks
  elements = grid_ui_model_get_elements(ui);

  // Initialize configuration and calibration
  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, 12);

  // Set up calibration limits for analog buttons (elements 8-23)
  for (int i = 8; i < 24; ++i) {
    assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &ui_button_state[i].limits) == 0);
  }

  // Wait for configuration to load from NVM
  while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
    vTaskDelay(1);
  }

  while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    vTaskDelay(1);
  }

  // Initialize encoder I2S interface with callback
  grid_esp32_encoder_init(enc, 1, octv_process_encoder);

  // Get encoder characteristics from hardware config
  uint8_t detent = grid_hwcfg_module_encoder_is_detent(&grid_sys_state);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

  for (uint8_t i = 0; i < GRID_MODULE_OCTV_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  // Initialize ADC with callback
  grid_esp32_adc_init(adc, octv_process_analog);

  // Initialize multiplexer with 8 positions (8 mux positions * 2 channels = 16 inputs)
  grid_esp32_adc_mux_init(adc, 8);

  // RevH modules are NOT mux-dependent (ADC timing independent of mux settling)
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

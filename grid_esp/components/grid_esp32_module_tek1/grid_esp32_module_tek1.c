/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_tek1.h"

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
#include "grid_ui_endless.h"
#include "grid_ui_lcd.h"
#include "grid_ui_system.h"

#include "grid_esp32_lcd.h"
#include "grid_esp32_port.h"
#include "grid_font.h"
#include "grid_gui.h"

#include "grid_lua_api.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_tek1";

static DRAM_ATTR uint8_t is_vsn_rev_h_8bit_hwcfg = 0;

#define GRID_MODULE_TEK1_POT_NUM 2

#define GRID_MODULE_TEK1_BUT_NUM 17

#define GRID_MODULE_TEK1_SMOL_BUT_NUM 8

static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;
static struct grid_ui_endless_sample DRAM_ATTR endless_sample = {0};

void IRAM_ATTR vsn1l_process_analog(void* user) {

#define X GRID_MUX_UNUSED
  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {8, 9, 8, 10, 8, 11, X, 12, 2, 0, 3, 1, 6, 4, 7, 5};
#undef X

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t element_index = multiplexer_lookup[lookup_index];

  if (element_index == GRID_MUX_UNUSED) {
    return;
  }

  assert(element_index < GRID_MODULE_TEK1_BUT_NUM);

  struct grid_ui_element* ele = &elements[element_index];

  if (!grid_asc_process(asc_state, lookup_index, result->value, &result->value)) {
    return;
  }

  if (element_index < 8) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);

  } else if (element_index < 9) {

    switch (lookup_index) {
    case 0: {
      endless_sample.phase_a = result->value;
    } break;
    case 2: {
      endless_sample.phase_b = result->value;
    } break;
    case 4: {
      endless_sample.button_value = result->value;
      grid_ui_endless_store_input(&grid_ui_state, element_index, endless_sample);
    } break;
    }
  } else if (element_index < 13 && is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);
  }
}

void IRAM_ATTR vsn1l_process_encoder(void* dma_buf) {

#define X GRID_MUX_UNUSED
  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {9, 10, 11, 12, X, X, X, X};
#undef X

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = bit * (1 << (GRID_AIN_INTERNAL_RESOLUTION - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if (idx >= 9 && idx < 13) {

      grid_ui_button_store_input(&grid_ui_state, idx, value);
    }
  }
}

void IRAM_ATTR vsn1r_process_analog(void* user) {

#define X GRID_MUX_UNUSED
  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {9, 8, 10, 8, 11, 8, 12, X, 2, 0, 3, 1, 6, 4, 7, 5};
#undef X

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t element_index = multiplexer_lookup[lookup_index];

  if (element_index == GRID_MUX_UNUSED) {
    return;
  }

  assert(element_index < GRID_MODULE_TEK1_BUT_NUM);

  struct grid_ui_element* ele = &elements[element_index];

  if (!grid_asc_process(asc_state, lookup_index, result->value, &result->value)) {
    return;
  }

  if (element_index < 8) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);

  } else if (element_index < 9) {

    switch (lookup_index) {
    case 1: {
      endless_sample.phase_a = result->value;
    } break;
    case 3: {
      endless_sample.phase_b = result->value;
    } break;
    case 5: {
      endless_sample.button_value = result->value;
      grid_ui_endless_store_input(&grid_ui_state, element_index, endless_sample);
    } break;
    }
  } else if (element_index < 13 && is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);
  }
}

void IRAM_ATTR vsn1r_process_encoder(void* dma_buf) {

#define X GRID_MUX_UNUSED
  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {X, X, X, X, 9, 10, 11, 12};
#undef X

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = (bit > 0) * (1 << (GRID_AIN_INTERNAL_RESOLUTION - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if (idx >= 9 && idx < 13) {

      grid_ui_button_store_input(&grid_ui_state, idx, value);
    }
  }
}

void IRAM_ATTR vsn2_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {13, 8, 14, 9, 15, 10, 16, 11, 2, 0, 3, 1, 6, 4, 7, 5};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t element_index = multiplexer_lookup[lookup_index];

  assert(element_index < GRID_MODULE_TEK1_BUT_NUM);

  struct grid_ui_element* ele = &elements[element_index];

  if (!grid_asc_process(asc_state, lookup_index, result->value, &result->value)) {
    return;
  }

  if (element_index < 8) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);

  } else if (is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(&grid_ui_state, element_index, result->value);
  }
}

void IRAM_ATTR vsn2_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {8, 9, 10, 11, 13, 14, 15, 16};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = (bit > 0) * (1 << (GRID_AIN_INTERNAL_RESOLUTION - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if ((idx >= 8 && idx < 12) || (idx >= 13 && idx < 17)) {

      grid_ui_button_store_input(&grid_ui_state, idx, value);
    }
  }
}

void grid_esp32_module_tek1_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal,
                                 struct grid_esp32_lcd_model* lcds) {

  // Allocate transfer buffer
  uint32_t width = LCD_HRES;
  uint32_t height = LCD_VRES;
  uint32_t lcd_tx_lines = 16;
  uint32_t lcd_tx_bytes = height * lcd_tx_lines * COLMOD_RGB888_BYTES;
  // uint8_t* xferbuf = malloc(lcd_tx_bytes);

  // Initialize LCD
  grid_esp32_lcd_spi_bus_init(lcd_tx_bytes);

  // Wait for the coprocessor to pull the LCD reset pin high
  while (!rp2040_active) {
    vTaskDelay(1);
  }

  bool is_tek1_reva = grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA;

  uint32_t hwcfg = grid_sys_get_hwcfg(sys);

  // Initialize LCD panel at index 0 for VSN1L
  if (grid_hwcfg_module_is_vsnl(sys) || is_tek1_reva) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[0];
    struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

    grid_esp32_lcd_panel_init(lcd, &elements[13], 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, &elements[13], 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Initialize LCD panel at index 1 for VSN1R
  if (grid_hwcfg_module_is_vsnr(sys)) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[1];
    struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

    grid_esp32_lcd_panel_init(lcd, &elements[13], 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, &elements[13], 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Initialize LCD panel at index 0 and 1 for VSN2
  if (grid_hwcfg_module_is_vsn2(sys)) {

    struct grid_esp32_lcd_model* lcds = grid_esp32_lcd_states;
    struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

    grid_esp32_lcd_panel_init(&lcds[0], &elements[12], 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&lcds[0], &elements[12], 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&lcds[0]);
    grid_esp32_lcd_panel_set_frctrl2(&lcds[0], LCD_FRCTRL_40HZ);

    grid_esp32_lcd_panel_init(&lcds[1], &elements[17], 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&lcds[1], &elements[17], 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&lcds[1]);
    grid_esp32_lcd_panel_set_frctrl2(&lcds[1], LCD_FRCTRL_40HZ);
  }

  // Mark the LCD as ready
  grid_esp32_lcd_set_ready(true);

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  // Initialize button states for all button and endless elements
  for (int i = 0; i < ui->element_list_length - 1; ++i) { // -1 to skip system element
    struct grid_ui_element* ele = &ui->element_list[i];

    if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      grid_ui_button_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENDLESS) {
      struct grid_ui_endless_state* endless_state = (struct grid_ui_endless_state*)ele->primary_state;
      endless_state->adc_bit_depth = GRID_AIN_INTERNAL_RESOLUTION;
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->secondary_state;
      grid_ui_button_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
    }
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);
  if (grid_hwcfg_module_is_rev_h(sys)) {
    grid_asc_array_set_factors(asc_state, 16, 8, 8, 1);
  }

  elements = grid_ui_model_get_elements(ui);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  if (grid_hwcfg_module_is_rev_h(sys)) {

    for (int i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {
      struct grid_ui_element* ele = &ui->element_list[i];
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
    }

    assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
    grid_ui_bulk_flush(ui);
  }

  grid_process_encoder_t process_encoder = NULL;

  if (grid_hwcfg_module_is_vsnl(sys) || is_tek1_reva) {
    process_encoder = vsn1l_process_encoder;
    grid_esp32_adc_init(adc, vsn1l_process_analog);
  } else if (grid_hwcfg_module_is_vsnr(sys)) {
    process_encoder = vsn1r_process_encoder;
    grid_esp32_adc_init(adc, vsn1r_process_analog);
  } else if (grid_hwcfg_module_is_vsn2(sys)) {
    process_encoder = vsn2_process_encoder;
    grid_esp32_adc_init(adc, vsn2_process_analog);
  }

  is_vsn_rev_h_8bit_hwcfg = 1 - grid_platform_get_hwcfg_bit(16);

  if (!is_vsn_rev_h_8bit_hwcfg) {

    grid_esp32_encoder_init(&grid_esp32_encoder_state, 10, process_encoder);
  }

  grid_platform_mux_init(0b11111111);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

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

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_endless_state* DRAM_ATTR new_endless_state = NULL;
static struct grid_ui_endless_state* DRAM_ATTR old_endless_state = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR vsn1l_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {8, 9, 8, 10, 8, 11, -1, 12, 2, 0, 3, 1, 6, 4, 7, 5};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 8) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);

  } else if (mux_position < 9) {

    switch (lookup_index) {
    case 0: {

      new_endless_state[0].phase_a = result->value;
    } break;
    case 2: {

      new_endless_state[0].phase_b = result->value;
    } break;
    case 4: {

      new_endless_state[0].button_value = result->value;
      grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
      grid_ui_endless_store_input(ele, mux_position, 12, &new_endless_state[0], &old_endless_state[0]);
    } break;
    }
  } else if (mux_position < 13 && is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
  }
}

void IRAM_ATTR vsn1l_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {9, 10, 11, 12, -1, -1, -1, -1};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = bit * (1 << (12 - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if (idx >= 9 && idx < 13) {

      grid_ui_button_store_input(ele, &ui_button_state[idx], value, 12);
    }
  }
}

void IRAM_ATTR vsn1r_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {9, 8, 10, 8, 11, 8, 12, -1, 2, 0, 3, 1, 6, 4, 7, 5};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 8) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);

  } else if (mux_position < 9) {

    switch (lookup_index) {
    case 1: {

      new_endless_state[0].phase_a = result->value;
    } break;
    case 3: {

      new_endless_state[0].phase_b = result->value;
    } break;
    case 5: {

      new_endless_state[0].button_value = result->value;
      grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
      grid_ui_endless_store_input(ele, mux_position, 12, &new_endless_state[0], &old_endless_state[0]);
    } break;
    }
  } else if (mux_position < 13 && is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
  }
}

void IRAM_ATTR vsn1r_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {-1, -1, -1, -1, 9, 10, 11, 12};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = (bit > 0) * (1 << (12 - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if (idx >= 9 && idx < 13) {

      grid_ui_button_store_input(ele, &ui_button_state[idx], value, 12);
    }
  }
}

void IRAM_ATTR vsn2_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {13, 8, 14, 9, 15, 10, 16, 11, 2, 0, 3, 1, 6, 4, 7, 5};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 8) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);

  } else if (is_vsn_rev_h_8bit_hwcfg) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
  }
}

void IRAM_ATTR vsn2_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_TEK1_SMOL_BUT_NUM] = {9, 10, 11, 12, 13, 14, 15, 16};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t i = 0; i < GRID_MODULE_TEK1_SMOL_BUT_NUM; ++i) {

    uint8_t bit = bytes[i / 8] & (1 << i);
    uint16_t value = (bit > 0) * (1 << (12 - 1));
    uint8_t idx = encoder_lookup[i];
    struct grid_ui_element* ele = &elements[idx];

    if (idx >= 9 && idx < 17) {

      grid_ui_button_store_input(ele, &ui_button_state[idx], value, 12);
    }
  }
}

void grid_esp32_module_tek1_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal,
                                 struct grid_esp32_lcd_model* lcds) {

  // Allocate transfer buffer
  uint32_t height = LCD_VRES;
  uint32_t lcd_tx_lines = 16;
  uint32_t lcd_tx_bytes = height * lcd_tx_lines * COLMOD_RGB888_BYTES;

  // Initialize LCD
  grid_esp32_lcd_spi_bus_init(lcd_tx_bytes);

  // Wait for the coprocessor to pull the LCD reset pin high
  vTaskDelay(pdMS_TO_TICKS(500));

  uint32_t hwcfg = grid_sys_get_hwcfg(sys);

  // Initialize LCD panel at index 0, if necessary
  if (hwcfg == GRID_MODULE_TEK1_RevA || hwcfg == GRID_MODULE_VSN1L_RevA || hwcfg == GRID_MODULE_VSN1L_RevB || hwcfg == GRID_MODULE_VSN1L_RevH || hwcfg == GRID_MODULE_VSN2_RevA ||
      hwcfg == GRID_MODULE_VSN2_RevB || hwcfg == GRID_MODULE_VSN2_RevH) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[0];
    struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

    grid_esp32_lcd_panel_init(lcd, &elements[13], 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, &elements[13], 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Initialize LCD panel at index 1, if necessary
  if (hwcfg == GRID_MODULE_VSN1R_RevA || hwcfg == GRID_MODULE_VSN1R_RevB || hwcfg == GRID_MODULE_VSN1R_RevH || hwcfg == GRID_MODULE_VSN2_RevA || hwcfg == GRID_MODULE_VSN2_RevB ||
      hwcfg == GRID_MODULE_VSN2_RevH) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[1];
    struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

    grid_esp32_lcd_panel_init(lcd, &elements[13], 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, &elements[13], 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Mark the LCD as ready
  grid_esp32_lcd_set_ready(true);

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_TEK1_BUT_NUM * sizeof(struct grid_ui_button_state));
  new_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK1_POT_NUM * sizeof(struct grid_ui_endless_state));
  old_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK1_POT_NUM * sizeof(struct grid_ui_endless_state));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_TEK1_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(new_endless_state, 0, GRID_MODULE_TEK1_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(old_endless_state, 0, GRID_MODULE_TEK1_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_TEK1_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);
  if (grid_hwcfg_module_is_rev_h(sys)) {
    grid_asc_array_set_factors(asc_state, 16, 8, 8, 1);
  }

  elements = grid_ui_model_get_elements(ui);

  grid_config_init(conf, cal);

  if (grid_hwcfg_module_is_rev_h(sys)) {

    struct grid_cal_but* cal_but = &cal->button;
    grid_cal_but_init(cal_but, ui->element_list_length);
    for (int i = 0; i < 8; ++i) {
      grid_cal_but_enable_set(cal_but, i, &ui_button_state[i]);
    }

    while (grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL)) {
      vTaskDelay(1);
    }

    while (grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
      vTaskDelay(1);
    }
  }

  grid_process_encoder_t process_encoder = NULL;

  if (grid_sys_get_hwcfg(sys) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1L_RevA || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1L_RevB ||
      grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1L_RevH) {
    process_encoder = vsn1l_process_encoder;
    grid_esp32_adc_init(adc, vsn1l_process_analog);
  } else if (grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1R_RevB || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1R_RevH) {
    process_encoder = vsn1r_process_encoder;
    grid_esp32_adc_init(adc, vsn1r_process_analog);
  } else if (grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN2_RevB || grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN2_RevH) {
    process_encoder = vsn2_process_encoder;
    grid_esp32_adc_init(adc, vsn2_process_analog);
  }

  is_vsn_rev_h_8bit_hwcfg = 1 - grid_platform_get_hwcfg_bit(16);

  if (!is_vsn_rev_h_8bit_hwcfg) {

    grid_esp32_encoder_init(&grid_esp32_encoder_state, 10, process_encoder);
  }

  grid_esp32_adc_mux_init(adc, 8);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(adc, mux_dependent);
}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_vsnx.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_endless.h"
#include "grid_ui_lcd.h"

#include "grid_esp32_lcd.h"
#include "grid_esp32_port.h"
#include "grid_font.h"
#include "grid_gui.h"

#include "grid_lua_api.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_vsnx";

#define GRID_MODULE_VSNX_BUTTON_COUNT 8
#define GRID_MODULE_VSNX_MINIBUTTON_COUNT 8
#define GRID_MODULE_VSNX_ASC_FACTOR 8

static struct grid_ui_model* DRAM_ATTR ui_ptr = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
#define GRID_MODULE_VSNX_ENDLESS_MAXCOUNT 2
static struct grid_ui_endless_sample DRAM_ATTR endless_sample[GRID_MODULE_VSNX_ENDLESS_MAXCOUNT] = {0};

#define X GRID_MUX_UNUSED
static DRAM_ATTR const uint8_t vsn1l_mux_element_lookup[2][8] = {
    {X, X, X, X, 0, 1, 4, 5},
    {8, 8, 8, X, 2, 3, 6, 7},
};
static DRAM_ATTR const uint8_t vsn1r_mux_element_lookup[2][8] = {
    {8, 8, 8, X, 0, 1, 4, 5},
    {X, X, X, X, 2, 3, 6, 7},
};
static DRAM_ATTR const uint8_t vsn2_mux_element_lookup[2][8] = {
    {X, X, X, X, 0, 1, 4, 5},
    {X, X, X, X, 2, 3, 6, 7},
};
static DRAM_ATTR const uint8_t tek2_mux_element_lookup[2][8] = {
    {8, 8, 8, X, 0, 1, 4, 5},
    {9, 9, 9, X, 2, 3, 6, 7},
};
#undef X

// Pointer to 2D lookup table [2][8], indexed as [channel][mux_state]
static DRAM_ATTR const uint8_t (*vsnx_mux_lookup)[8] = NULL;

void IRAM_ATTR vsnx_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = vsnx_mux_lookup[result->channel][result->mux_state];

  if (element_index == GRID_MUX_UNUSED) {
    return;
  }

  if (!grid_asc_process(&asc_state[element_index], result->value, &result->value)) {
    return;
  }

  if (element_index < GRID_MODULE_VSNX_BUTTON_COUNT) {

    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_button_store_input(grid_ui_button_get_state(ele), result->value);

  } else {

    uint8_t endless_idx = element_index - GRID_MODULE_VSNX_BUTTON_COUNT;
    struct grid_ui_endless_sample* sample_ptr = &endless_sample[endless_idx];

    if (result->mux_state == 0) {
      sample_ptr->phase_a = result->value;
    } else if (result->mux_state == 1) {
      sample_ptr->phase_b = result->value;
    } else if (result->mux_state == 2) {
      sample_ptr->button_value = result->value;
      struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
      grid_ui_endless_store_input(grid_ui_endless_get_state(ele), *sample_ptr);
    }
  }
}

#define X GRID_MUX_UNUSED
static DRAM_ATTR const uint8_t vsn1l_minibutton_lookup[GRID_MODULE_VSNX_MINIBUTTON_COUNT] = {9, 10, 11, 12, X, X, X, X};
static DRAM_ATTR const uint8_t vsn1r_minibutton_lookup[GRID_MODULE_VSNX_MINIBUTTON_COUNT] = {X, X, X, X, 9, 10, 11, 12};
static DRAM_ATTR const uint8_t vsn2_minibutton_lookup[GRID_MODULE_VSNX_MINIBUTTON_COUNT] = {8, 9, 10, 11, 13, 14, 15, 16};
static DRAM_ATTR const uint8_t tek2_minibutton_lookup[GRID_MODULE_VSNX_MINIBUTTON_COUNT] = {X, X, X, X, X, X, X, X};
#undef X

static DRAM_ATTR const uint8_t* vsnx_minibutton_lookup = NULL;

void IRAM_ATTR vsnx_process_minibutton(struct grid_encoder_result* result) {

  uint8_t minibutton_state_bm = result->data[0];

  for (uint8_t i = 0; i < GRID_MODULE_VSNX_MINIBUTTON_COUNT; ++i) {

    uint8_t element_index = vsnx_minibutton_lookup[i];

    if (element_index != GRID_MUX_UNUSED) {
      uint8_t bit = (minibutton_state_bm >> i) & 1;
      struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
      grid_ui_button_store_input(grid_ui_button_get_state(ele), bit);
    }
  }
}

static void init_lcd_panel(struct grid_ui_element* element, uint8_t lcd_index, uint8_t cs_index) {
  struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[lcd_index];
  grid_esp32_lcd_panel_init(lcd, element, cs_index, GRID_LCD_CLK_SLOW);
  grid_esp32_lcd_panel_init(lcd, element, cs_index, GRID_LCD_CLK_FAST);
  grid_esp32_lcd_panel_reset(lcd);
  grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
}

static void vsnx_lcd_init(struct grid_sys_model* sys, struct grid_ui_model* ui) {

  uint32_t lcd_tx_bytes = LCD_VRES * 16 * COLMOD_RGB888_BYTES;
  grid_esp32_lcd_spi_bus_init(lcd_tx_bytes);

  // Wait for the coprocessor to pull the LCD reset pin high
  while (!rp2040_active) {
    vTaskDelay(1);
  }

  struct grid_ui_element* elements = grid_ui_model_get_elements(ui);

  if (grid_hwcfg_module_is_vsnl(sys)) {
    init_lcd_panel(&elements[13], 0, 0);
  } else if (grid_hwcfg_module_is_vsnr(sys)) {
    init_lcd_panel(&elements[13], 1, 1);
  } else if (grid_hwcfg_module_is_vsn2(sys)) {
    init_lcd_panel(&elements[12], 0, 0);
    init_lcd_panel(&elements[17], 1, 1);
  }

  grid_esp32_lcd_set_ready(true);
}

void grid_esp32_module_vsnx_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal,
                                 struct grid_esp32_lcd_model* lcds) {

  ui_ptr = ui;

  bool rev_h = grid_hwcfg_module_is_rev_h(sys);

  if (!grid_hwcfg_module_is_tek2(sys)) {
    vsnx_lcd_init(sys, ui);
  }

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON && i < GRID_MODULE_VSNX_BUTTON_COUNT) {
      grid_ui_button_state_init(grid_ui_button_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      grid_ui_button_state_init(grid_ui_button_get_state(ele), 1, GRID_BUTTON_THRESHOLD, 0.0);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENDLESS) {
      grid_ui_endless_state_init(grid_ui_endless_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
    }
  }

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON && i < GRID_MODULE_VSNX_BUTTON_COUNT) {
      grid_asc_set_factor(&asc_state[i], GRID_MODULE_VSNX_ASC_FACTOR);
      if (rev_h) {
        grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &grid_ui_button_get_state(ele)->limits);
      }
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENDLESS) {
      // ASC factor must be 1 for ENDLESS to pass through every sample!
      // This limitation is due to asc not being able to process multidimensional samples!
      grid_asc_set_factor(&asc_state[i], 1);
    }
  }

  if (rev_h) {
    assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
    grid_ui_bulk_flush(ui);
  }

  if (grid_hwcfg_module_is_vsnl(sys)) {
    vsnx_mux_lookup = vsn1l_mux_element_lookup;
    vsnx_minibutton_lookup = vsn1l_minibutton_lookup;
  } else if (grid_hwcfg_module_is_vsnr(sys)) {
    vsnx_mux_lookup = vsn1r_mux_element_lookup;
    vsnx_minibutton_lookup = vsn1r_minibutton_lookup;
  } else if (grid_hwcfg_module_is_vsn2(sys)) {
    vsnx_mux_lookup = vsn2_mux_element_lookup;
    vsnx_minibutton_lookup = vsn2_minibutton_lookup;
  } else if (grid_hwcfg_module_is_tek2(sys)) {
    vsnx_mux_lookup = tek2_mux_element_lookup;
    vsnx_minibutton_lookup = tek2_minibutton_lookup;
  }

  // Encoder driver is used to read minibuttons via shift registers
  uint8_t transfer_length = 2;
  uint32_t clock_rate = 200 * I2S_DATA_BIT_WIDTH_32BIT * 4;
  grid_esp32_encoder_init(&grid_esp32_encoder_state, transfer_length, clock_rate, vsnx_process_minibutton);

  uint8_t mux_positions = grid_hwcfg_module_is_tek2(sys) ? 0b11110111 : 0b11111111;
  uint8_t mux_dependent = !rev_h;
  grid_esp32_adc_init(adc, mux_positions, mux_dependent, vsnx_process_analog);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);
  grid_esp32_adc_start(adc);
}

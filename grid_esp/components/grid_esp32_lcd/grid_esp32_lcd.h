/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "grid_gui.h"
#include "grid_sys.h"

#include "driver/gpio.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum grid_lcd_clock_t {
  GRID_LCD_CLK_SLOW = 0,
  GRID_LCD_CLK_FAST,
  GRID_LCD_CLK_COUNT,
};

struct grid_esp32_lcd_model {

  esp_lcd_panel_handle_t panel[2][GRID_LCD_CLK_COUNT];
  esp_lcd_panel_io_handle_t panel_io[2][GRID_LCD_CLK_COUNT];
  int cs_gpio_num[2];
  uint8_t tx_ready[2];
};

extern struct grid_esp32_lcd_model grid_esp32_lcd_state;

void grid_esp32_lcd_spi_bus_init(struct grid_esp32_lcd_model* lcd);
void grid_esp32_lcd_panel_init(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, enum grid_lcd_clock_t);
bool grid_esp32_lcd_panel_active(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index);
void grid_esp32_lcd_panel_reset(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index);
bool grid_esp32_lcd_panel_tx_ready(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index);
int grid_esp32_lcd_draw_bitmap_blocking(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint16_t x, uint16_t y, uint16_t width, uint16_t height, void* framebuffer);
int grid_esp32_lcd_set_madctl(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint8_t madctl);
int grid_esp32_lcd_set_frctrl2(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint8_t frctrl);
int grid_esp32_lcd_get_scanline(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint16_t offset, uint16_t* scanline);

bool grid_esp32_lcd_scan_in_range(int max_excl, int start, int length, int x);

#ifdef __cplusplus
}
#endif

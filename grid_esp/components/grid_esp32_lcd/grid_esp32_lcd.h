/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_lcd_model {

  uint8_t foo;
  void* lcd_handle;
};

extern struct grid_esp32_lcd_model grid_esp32_lcd_state;

void grid_esp32_lcd_model_init(struct grid_esp32_lcd_model* lcd);
void grid_esp32_lcd_hardware_init(struct grid_esp32_lcd_model* lcd);

int grid_esp32_lcd_draw_bitmap(struct grid_esp32_lcd_model* lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, void* framebuffer);

#ifdef __cplusplus
}
#endif

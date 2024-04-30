/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_lcd.h"
#include "grid_gui.h"

#include "driver/gpio.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"

#define LCD_SPI_HOST SPI3_HOST
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL (!LCD_BK_LIGHT_ON_LEVEL)
#define PIN_NUM_MOSI 48 /*2*/
#define PIN_NUM_CLK 47  /*1*/
#define PIN_NUM_CS 33   /*5*/
#define PIN_NUM_DC 34   /*4*/
#define PIN_NUM_RST -1  /*1*/
#define PIN_NUM_BCKL -1 /*6*/
#define LCD_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 320
#define LCD_VRES 240
#define LCD_COLOR_SPACE LCD_RGB_ELEMENT_ORDER_RGB
#define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X true
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#define LCD_TRANSFER_SIZE (320 * 240 * 3)
#define LCD_FLUSH_CALLBACK lcd_flush_ready

volatile int lcd_flush_ready = 0;

bool ready_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
  lcd_flush_ready = 1;
  return 1;
}

esp_lcd_panel_io_callbacks_t lcd_callbacks = {.on_color_trans_done = ready_cb};

int grid_esp32_lcd_draw_bitmap_blocking(struct grid_esp32_lcd_model* lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, void* framebuffer) {

  lcd_flush_ready = 0;
  esp_lcd_panel_draw_bitmap((esp_lcd_panel_handle_t)lcd->lcd_handle, x, y, x + width, y + height, framebuffer);
  while (!lcd_flush_ready)
    ;

  return 0;
}

// global so it can be used after init
esp_lcd_panel_handle_t lcd_handle;

struct grid_esp32_lcd_model grid_esp32_lcd_state;

void grid_esp32_lcd_model_init(struct grid_esp32_lcd_model* lcd) { lcd->foo = 255; }

void grid_esp32_lcd_hardware_init(struct grid_esp32_lcd_model* lcd) {

  // pinMode(PIN_NUM_BCKL, OUTPUT);
  spi_bus_config_t bus_config;
  memset(&bus_config, 0, sizeof(bus_config));
  bus_config.sclk_io_num = PIN_NUM_CLK;
  bus_config.mosi_io_num = PIN_NUM_MOSI;
  bus_config.miso_io_num = -1;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = LCD_TRANSFER_SIZE + 8;

  // Initialize the SPI bus on LCD_SPI_HOST
  spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);

  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config;
  memset(&io_config, 0, sizeof(io_config));
  io_config.dc_gpio_num = PIN_NUM_DC, io_config.cs_gpio_num = PIN_NUM_CS, io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ, io_config.lcd_cmd_bits = 8, io_config.lcd_param_bits = 8, io_config.spi_mode = 0,
  io_config.trans_queue_depth = 10, io_config.on_color_trans_done = NULL;
  // Attach the LCD to the SPI bus
  esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle);

  lcd_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config;
  memset(&panel_config, 0, sizeof(panel_config));

  panel_config.reset_gpio_num = PIN_NUM_RST;

  panel_config.color_space = LCD_COLOR_SPACE;
  panel_config.bits_per_pixel = 18; // 6 bit per pixel

  // Initialize the LCD configuration
  LCD_PANEL(io_handle, &panel_config, &lcd_handle);

  // Turn off backlight to avoid unpredictable display on
  // the LCD screen while initializing
  // the LCD panel driver. (Different LCD screens may need different levels)
  // digitalWrite(PIN_NUM_BCKL, LCD_BK_LIGHT_OFF_LEVEL);

  // Reset the display
  esp_lcd_panel_reset(lcd_handle);

  // Initialize LCD panel
  esp_lcd_panel_init(lcd_handle);

  esp_lcd_panel_swap_xy(lcd_handle, LCD_SWAP_XY);
  esp_lcd_panel_set_gap(lcd_handle, LCD_GAP_X, LCD_GAP_Y);
  esp_lcd_panel_mirror(lcd_handle, LCD_MIRROR_X, LCD_MIRROR_Y);
  esp_lcd_panel_invert_color(lcd_handle, LCD_INVERT_COLOR);
  // Turn on the screen
  esp_lcd_panel_disp_off(lcd_handle, false);

  // Turn on backlight (Different LCD screens may need different levels)
  // digitalWrite(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);

  esp_lcd_panel_io_register_event_callbacks(io_handle, &lcd_callbacks, NULL);

  lcd->lcd_handle = (void*)lcd_handle;
}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_lcd.h"

#define LCD_SPI_HOST SPI3_HOST
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL (!LCD_BK_LIGHT_ON_LEVEL)
#define PIN_NUM_MOSI 48 /*2*/
#define PIN_NUM_CLK 47  /*1*/
#define PIN_NUM_CS0 18  /*5*/
#define PIN_NUM_CS1 16  /*5*/
#define PIN_NUM_DC 34   /*4*/
#define PIN_NUM_RST -1  /*1*/
#define PIN_NUM_BCKL -1 /*6*/
#define LCD_NEW_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 320
#define LCD_VRES 240
#define LCD_LINES LCD_HRES
#define LCD_COLOR_SPACE LCD_RGB_ELEMENT_ORDER_RGB
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X true
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#define LCD_TRANSFER_SIZE (320 * 4 * 3)
#define LCD_FLUSH_CALLBACK lcd_flush_ready

struct grid_esp32_lcd_model grid_esp32_lcd_state = {0};

bool color_trans_done_0(void* panel_io, void* edata, void* user_ctx) {

  grid_esp32_lcd_state.tx_ready[0] = 1;

  return true;
}

bool color_trans_done_1(void* panel_io, void* edata, void* user_ctx) {

  grid_esp32_lcd_state.tx_ready[1] = 1;

  return true;
}

void grid_esp32_lcd_spi_bus_init(struct grid_esp32_lcd_model* lcd) {

  spi_bus_config_t bus_config;
  memset(&bus_config, 0, sizeof(bus_config));
  bus_config.sclk_io_num = PIN_NUM_CLK;
  bus_config.mosi_io_num = PIN_NUM_MOSI;
  bus_config.miso_io_num = -1;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = LCD_TRANSFER_SIZE + 8;

  // Initialize the SPI bus
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
}

void grid_esp32_lcd_panel_init(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, enum grid_lcd_clock_t clock) {

  assert(lcd_index < 2);
  assert(clock < GRID_LCD_CLK_COUNT);

  esp_lcd_panel_io_spi_config_t io_config;
  memset(&io_config, 0, sizeof(io_config));

  io_config.cs_gpio_num = -1;

  if (grid_hwcfg_module_is_vsnx_rev_a(&grid_sys_state)) {
    lcd->cs_gpio_num[lcd_index] = (lcd_index ? 16 : 33);
  } else {
    lcd->cs_gpio_num[lcd_index] = (lcd_index ? PIN_NUM_CS1 : PIN_NUM_CS0);
  }

  gpio_set_direction(lcd->cs_gpio_num[lcd_index], GPIO_MODE_OUTPUT);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  // Panel starts out ready for transmission
  lcd->tx_ready[lcd_index] = 1;

  io_config.dc_gpio_num = PIN_NUM_DC;
  io_config.spi_mode = 0;

  switch (clock) {
  case GRID_LCD_CLK_SLOW: {
    io_config.pclk_hz = 4 * 1000 * 1000;
  } break;
  case GRID_LCD_CLK_FAST: {
    io_config.pclk_hz = 65 * 1000 * 1000;
  } break;
  default: {
    assert(0);
  }
  }

  io_config.trans_queue_depth = 10;
  io_config.on_color_trans_done = NULL;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  io_config.flags.sio_mode = 1;

  // Configure generic panel IO, add it to the bus
  esp_lcd_panel_io_handle_t panel_io_handle = NULL;
  esp_lcd_spi_bus_handle_t bus_handle = LCD_SPI_HOST;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(bus_handle, &io_config, &panel_io_handle));

  static bool (*trans_done[2])(void*, void*, void*) = {
      color_trans_done_0,
      color_trans_done_1,
  };

  esp_lcd_panel_io_callbacks_t callbacks = {
      .on_color_trans_done = trans_done[lcd_index],
  };

  // Register panel IO callbacks
  ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(panel_io_handle, &callbacks, lcd));

  esp_lcd_panel_dev_config_t panel_config;
  memset(&panel_config, 0, sizeof(panel_config));
  panel_config.reset_gpio_num = PIN_NUM_RST;
  panel_config.color_space = LCD_COLOR_SPACE;
  panel_config.bits_per_pixel = 18;

  // Configure specific panel IO
  esp_lcd_panel_handle_t panel_handle = NULL;
  ESP_ERROR_CHECK(LCD_NEW_PANEL(panel_io_handle, &panel_config, &panel_handle));

  // Assign handles
  lcd->panel[lcd_index][clock] = panel_handle;
  lcd->panel_io[lcd_index][clock] = panel_io_handle;
}

bool grid_esp32_lcd_panel_active(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index) { return lcd->panel[lcd_index][GRID_LCD_CLK_SLOW] || lcd->panel[lcd_index][GRID_LCD_CLK_FAST]; }

void grid_esp32_lcd_panel_reset(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index) {

  esp_lcd_panel_handle_t handle = lcd->panel[lcd_index][GRID_LCD_CLK_FAST];

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_reset(handle);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_init(handle);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_swap_xy(handle, LCD_SWAP_XY);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_set_gap(handle, LCD_GAP_X, LCD_GAP_Y);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  bool mirror_x = lcd_index ? !LCD_MIRROR_X : LCD_MIRROR_X;
  bool mirror_y = lcd_index ? !LCD_MIRROR_Y : LCD_MIRROR_Y;
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  // esp_lcd_panel_mirror(handle, mirror_x, mirror_y);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_invert_color(handle, LCD_INVERT_COLOR);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);
  esp_lcd_panel_disp_off(handle, false);
  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);
}

bool grid_esp32_lcd_panel_tx_ready(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index) { return lcd->tx_ready[lcd_index]; }

int grid_esp32_lcd_draw_bitmap_blocking(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint16_t x, uint16_t y, uint16_t width, uint16_t height, void* framebuffer) {

  esp_lcd_panel_handle_t handle = lcd->panel[lcd_index][GRID_LCD_CLK_FAST];

  if (handle == NULL) {
    return 1;
  }

  lcd->tx_ready[lcd_index] = 0;

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);

  esp_err_t err = esp_lcd_panel_draw_bitmap(handle, x, y, x + width, y + height, framebuffer);
  if (err != ESP_OK) {

    grid_platform_printf("err: %d\n", err);
  }

  while (lcd->tx_ready[lcd_index] == 0) {
  }

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  return 0;
}

int grid_esp32_lcd_set_madctl(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint8_t madctl) {

  esp_lcd_panel_io_handle_t handle = lcd->panel_io[lcd_index][GRID_LCD_CLK_SLOW];

  if (handle == NULL) {
    return 1;
  }

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);

  uint8_t tx[1] = {
      madctl,
  };
  esp_err_t err = esp_lcd_panel_io_tx_param(handle, LCD_CMD_MADCTL, tx, 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  return err != ESP_OK;
}

int grid_esp32_lcd_set_frctrl2(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint8_t frctrl) {

  esp_lcd_panel_io_handle_t handle = lcd->panel_io[lcd_index][GRID_LCD_CLK_SLOW];

  if (handle == NULL) {
    return 1;
  }

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);

  uint8_t tx[1] = {
      frctrl,
  };
  esp_err_t err = esp_lcd_panel_io_tx_param(handle, 0xc6, tx, 1);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  return err != ESP_OK;
}

int grid_esp32_lcd_get_scanline(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, uint16_t offset, uint16_t* scanline) {

  esp_lcd_panel_io_handle_t handle = lcd->panel_io[lcd_index][GRID_LCD_CLK_SLOW];

  if (handle == NULL) {
    return 1;
  }

  assert(lcd->tx_ready[lcd_index]);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 0);

  uint8_t rx[3] = {0};
  esp_err_t err = esp_lcd_panel_io_rx_param(handle, LCD_CMD_GDCAN, rx, 3);

  gpio_set_level(lcd->cs_gpio_num[lcd_index], 1);

  if (err != ESP_OK) {
    return 1;
  }

  uint16_t scl = (rx[1] << 1) | (rx[2] >> 7);
  *scanline = scl < offset ? LCD_LINES : scl - offset;

  return 0;
}

static bool in_range_excl(int min, int max, int a, int b, int x) {

  assert(min < max);
  assert(min <= a && a < max);
  assert(min <= b && b < max);
  assert(min <= x && x < max);

  if (a <= b) {

    return a <= x && x < b;
  } else {

    return (min <= x && x < b) || (a <= x && x < max);
  }
}

static bool in_range_ahead_excl(int min, int max, int a, int len, int x) {

  assert(len >= 0);

  int range_len = max - min;

  if (len == range_len) {
    return true;
  }

  assert(len < range_len);

  int start = ((a - min + range_len) % range_len) + min;

  int end = ((a + len - min + range_len) % range_len) + min;

  return in_range_excl(min, max, start, end, x);
}

bool grid_esp32_lcd_scan_in_range(int max_excl, int start, int length, int x) { return in_range_ahead_excl(0, max_excl, start, length, x); }

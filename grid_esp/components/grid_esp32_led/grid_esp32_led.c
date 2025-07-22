/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_led.h"

#define RMT_LED_STRIP_RESOLUTION_HZ                                                                                                                                                                    \
  10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high
           // resolution)

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t* r, uint32_t* g, uint32_t* b) {
  h %= 360; // h -> [0,360]
  uint32_t rgb_max = v * 2.55f;
  uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

  uint32_t i = h / 60;
  uint32_t diff = h % 60;

  // RGB adjustment amount by hue
  uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

  switch (i) {
  case 0:
    *r = rgb_max;
    *g = rgb_min + rgb_adj;
    *b = rgb_min;
    break;
  case 1:
    *r = rgb_max - rgb_adj;
    *g = rgb_max;
    *b = rgb_min;
    break;
  case 2:
    *r = rgb_min;
    *g = rgb_max;
    *b = rgb_min + rgb_adj;
    break;
  case 3:
    *r = rgb_min;
    *g = rgb_max - rgb_adj;
    *b = rgb_max;
    break;
  case 4:
    *r = rgb_min + rgb_adj;
    *g = rgb_min;
    *b = rgb_max;
    break;
  default:
    *r = rgb_max;
    *g = rgb_min;
    *b = rgb_max - rgb_adj;
    break;
  }
}

static const char* TAG = "LED";

static void led_init(rmt_encoder_handle_t* led_encoder, rmt_channel_handle_t* led_chan, uint8_t led_gpio) {

  ESP_LOGI(TAG, "Led Pin: %d", led_gpio);

  rmt_tx_channel_config_t tx_chan_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
      .gpio_num = led_gpio,
      .flags.with_dma = 1,
      .mem_block_symbols = grid_led_get_led_count(&grid_led_state) * 24,
      .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
      .trans_queue_depth = 1,
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, led_chan));

  ESP_LOGI(TAG, "Install led strip encoder");
  led_strip_encoder_config_t encoder_config = {
      .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
  };

  ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, led_encoder));
  ESP_ERROR_CHECK(rmt_enable(*led_chan));
}

void grid_esp32_led_task(void* arg) {

  uint8_t led_pin = grid_led_get_pin(&grid_led_state);

  rmt_channel_handle_t led_chan = NULL;
  rmt_encoder_handle_t led_encoder = NULL;
  led_init(&led_encoder, &led_chan, led_pin);

  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

  // Use semaphore to delay the usage of LED until UI is properly initialized

  // static uint32_t loopcounter = 0;

  while (1) {

    grid_led_tick(&grid_led_state);
    grid_led_render_framebuffer(&grid_led_state);

    const uint8_t* frame_buffer = grid_led_get_framebuffer_pointer(&grid_led_state);
    const uint32_t frame_buffer_size = grid_led_get_framebuffer_size(&grid_led_state);

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, frame_buffer, frame_buffer_size, &tx_config));

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  ESP_LOGI(TAG, "Deinit LED");

  // Wait to be deleted
  xSemaphoreGive(signaling_sem);
  vTaskSuspend(NULL);
}

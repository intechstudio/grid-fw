/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_led.h"

static const char* TAG = "LED";

static DRAM_ATTR rmt_channel_handle_t CHANNEL = NULL;
static DRAM_ATTR rmt_encoder_handle_t ENCODER = NULL;

void grid_esp32_led_start(uint8_t led_gpio) {

  const int resolution = 10000000;

  rmt_tx_channel_config_t tx_chan_cfg = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .gpio_num = led_gpio,
      .flags.with_dma = 1,
      .mem_block_symbols = grid_led_get_framebuffer_size(&grid_led_state) * 8,
      .resolution_hz = resolution,
      .trans_queue_depth = 1,
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_cfg, &CHANNEL));

  led_strip_encoder_config_t encoder_cfg = {
      .resolution = resolution,
  };
  ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_cfg, &ENCODER));

  ESP_ERROR_CHECK(rmt_enable(CHANNEL));
}

void grid_esp32_utask_led(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_led_tick(&grid_led_state);
  grid_led_render_framebuffer(&grid_led_state);

  const uint8_t* framebuf = grid_led_get_framebuffer_pointer(&grid_led_state);
  const uint32_t framebuf_size = grid_led_get_framebuffer_size(&grid_led_state);

  rmt_transmit_config_t tx_config = {
      .loop_count = 0, // no transfer loop
  };

  ESP_ERROR_CHECK(rmt_transmit(CHANNEL, ENCODER, framebuf, framebuf_size, &tx_config));
}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_encoder.h"

#include "driver/gpio.h"

struct grid_esp32_encoder_model DRAM_ATTR grid_esp32_encoder_state;

static IRAM_ATTR bool i2s_recv_callback(i2s_chan_handle_t handle, i2s_event_data_t* event, void* user_ctx) {

  struct grid_esp32_encoder_model* encoder = user_ctx;

  struct grid_encoder_result result = {.data = &((uint8_t*)event->dma_buf)[1], .length = encoder->transfer_length - 1};
  encoder->process_encoder(&result);

  return true;
}

void grid_esp32_encoder_init(struct grid_esp32_encoder_model* encoder, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder) {

  uint32_t bits_per_frame = I2S_DATA_BIT_WIDTH_32BIT * 4;
  uint32_t sample_rate = clock_rate / bits_per_frame;

  encoder->transfer_length = transfer_length;

  assert(process_encoder);
  encoder->process_encoder = process_encoder;

  i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  rx_chan_cfg.dma_frame_num = 1;

  ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &encoder->rx_chan));

  i2s_tdm_config_t rx_tdm_cfg = {
      .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(sample_rate),
      .slot_cfg = I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO, I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = GRID_ESP32_PINS_HWCFG_CLOCK,
              .ws = GRID_ESP32_PINS_HWCFG_SHIFT,
              .dout = I2S_GPIO_UNUSED,
              .din = GRID_ESP32_PINS_HWCFG_DATA,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = true,
                      .ws_inv = true,
                  },
          },
  };

  rx_tdm_cfg.slot_cfg.big_endian = true;

  // Use XTAL (40 MHz) instead of PLL (160 MHz) to lower the minimum achievable sample rate.
  // Highest mclk_multiple to further minimize the minimum sample rate:
  // mclk_div = sclk / (sample_rate * mclk_multiple) must be < 256
  // min_sample_rate = 40MHz / (255 * 1152) ≈ 136 Hz
  rx_tdm_cfg.clk_cfg.clk_src = I2S_CLK_SRC_XTAL;
  rx_tdm_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_1152;
  ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(encoder->rx_chan, &rx_tdm_cfg));

  i2s_event_callbacks_t cbs = {
      .on_recv = i2s_recv_callback,
      .on_recv_q_ovf = NULL,
      .on_sent = NULL,
      .on_send_q_ovf = NULL,
  };
  ESP_ERROR_CHECK(i2s_channel_register_event_callback(encoder->rx_chan, &cbs, encoder));

  ESP_ERROR_CHECK(i2s_channel_enable(encoder->rx_chan));
}

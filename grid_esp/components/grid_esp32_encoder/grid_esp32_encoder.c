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

  ++encoder->dma_frame_count;

  if (encoder->dma_frame_count < encoder->dma_frame_div) {
    return true;
  }

  encoder->dma_frame_count = 0;

  encoder->process_encoder(event->dma_buf);

  return true;
}

void grid_esp32_encoder_init(struct grid_esp32_encoder_model* encoder, uint32_t divider, grid_process_encoder_t process_encoder) {

  assert(GRID_ESP32_ENCODER_I2S_SRATE % divider == 0);
  encoder->dma_frame_div = divider;
  encoder->dma_frame_count = 0;

  assert(process_encoder);
  encoder->process_encoder = process_encoder;

  i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  rx_chan_cfg.dma_frame_num = 1;

  ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &encoder->rx_chan));

  i2s_tdm_config_t rx_tdm_cfg = {
      .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(GRID_ESP32_ENCODER_I2S_SRATE),
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

  rx_tdm_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512;
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

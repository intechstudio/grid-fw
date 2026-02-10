/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#define STB_HEXWAVE_IMPLEMENTATION
#include "grid_esp32_codec.h"

#include "driver/gpio.h"

struct grid_esp32_codec_model grid_esp32_codec_state;

#include "driver/i2s_tdm.h"
#define EXAMPLE_TDM_BCLK_IO1 GPIO_NUM_41 // I2S bit clock io number
#define EXAMPLE_TDM_WS_IO1 GPIO_NUM_40   // I2S word select io number
#define EXAMPLE_TDM_DOUT_IO1 GPIO_NUM_42 // I2S data out io number
#define EXAMPLE_TDM_DIN_IO1 I2S_GPIO_UNUSED

#define SAMPLE_RATE 16000
#define TDM_SLOTS 2
#define MAX_FRAMES 512

#define FREQ_START 220.0
#define FREQ_INCREMENT 2.0
#define FREQ_MAX 4000.0

static i2s_chan_handle_t tx_chan;
static double* double_buf = NULL;
static double current_freq = FREQ_START;

static IRAM_ATTR bool i2s_tx_sent_callback(i2s_chan_handle_t handle, i2s_event_data_t* event, void* user_ctx) {

  int num_frames = event->size / (TDM_SLOTS * sizeof(int16_t));

  hexwave_generate_samples(double_buf, num_frames, &grid_esp32_codec_state.osc, current_freq / SAMPLE_RATE);

  int16_t* dst = (int16_t*)event->dma_buf;
  for (int i = 0; i < num_frames; i++) {
    int16_t sample = (int16_t)(double_buf[i] * 0x7FFF);
    for (int s = 0; s < TDM_SLOTS; s++) {
      dst[i * TDM_SLOTS + s] = sample;
    }
  }

  current_freq += FREQ_INCREMENT;
  if (current_freq > FREQ_MAX) {
    current_freq = FREQ_START;
  }

  return false;
}

static void i2s_example_init_tdm_simplex(void) {
  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

  i2s_tdm_config_t tx_tdm_cfg = {
      .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_TDM_SLOT0 | I2S_TDM_SLOT1),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = EXAMPLE_TDM_BCLK_IO1,
              .ws = EXAMPLE_TDM_WS_IO1,
              .dout = EXAMPLE_TDM_DOUT_IO1,
              .din = EXAMPLE_TDM_DIN_IO1,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };
  tx_tdm_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512;
  ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(tx_chan, &tx_tdm_cfg));

  i2s_event_callbacks_t cbs = {
      .on_recv = NULL,
      .on_recv_q_ovf = NULL,
      .on_sent = i2s_tx_sent_callback,
      .on_send_q_ovf = NULL,
  };
  ESP_ERROR_CHECK(i2s_channel_register_event_callback(tx_chan, &cbs, NULL));
}

void grid_esp32_codec_deinit(void) {
  free(double_buf);
  hexwave_shutdown(NULL);
}

void grid_esp32_codec_init(void) {
  i2s_example_init_tdm_simplex();

  hexwave_init(12, 4, NULL);
  hexwave_create(&grid_esp32_codec_state.osc, 0, 0.5, 0.0, 0.0);

  double_buf = (double*)calloc(MAX_FRAMES, sizeof(double));
  assert(double_buf);

  // Preload initial data
  int preload_frames = 240;
  hexwave_generate_samples(double_buf, preload_frames, &grid_esp32_codec_state.osc, FREQ_START / SAMPLE_RATE);

  int16_t* preload_buf = (int16_t*)calloc(preload_frames * TDM_SLOTS, sizeof(int16_t));
  assert(preload_buf);

  for (int i = 0; i < preload_frames; i++) {
    int16_t sample = (int16_t)(double_buf[i] * 0x7FFF);
    for (int s = 0; s < TDM_SLOTS; s++) {
      preload_buf[i * TDM_SLOTS + s] = sample;
    }
  }

  size_t preload_size = preload_frames * TDM_SLOTS * sizeof(int16_t);
  size_t w_bytes = preload_size;

  while (w_bytes == preload_size) {
    ESP_ERROR_CHECK(i2s_channel_preload_data(tx_chan, preload_buf, preload_size, &w_bytes));
  }

  free(preload_buf);
}

uint8_t enabled = false;

void grid_esp32_codec_enable(void) {
  if (enabled == true) {
    return;
  }
  enabled = true;
  current_freq = FREQ_START;
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void grid_esp32_codec_disable(void) {
  if (enabled == false) {
    return;
  }
  enabled = false;
  i2s_channel_disable(tx_chan);
}

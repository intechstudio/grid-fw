/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#define STB_HEXWAVE_IMPLEMENTATION
#include "grid_esp32_codec.h"

#include "driver/gpio.h"
#include "grid_esp32_platform.h"

struct grid_esp32_codec_model grid_esp32_codec_state;

#include "driver/i2s_tdm.h"
#define EXAMPLE_TDM_BCLK_IO1 GPIO_NUM_41 // I2S bit clock io number
#define EXAMPLE_TDM_WS_IO1 GPIO_NUM_40   // I2S word select io number
#define EXAMPLE_TDM_DOUT_IO1 GPIO_NUM_42 // I2S data out io number
#define EXAMPLE_TDM_DIN_IO1 I2S_GPIO_UNUSED

#define SAMPLE_RATE 16000
#define TDM_SLOTS 2
#define FREQ_DEFAULT 440.0
#define VOLUME_DEFAULT 0.5

uint32_t cycles_elapsed = 0;
uint32_t us_elapsed = 0;
int num_frames = 0;

static i2s_chan_handle_t tx_chan;

static IRAM_ATTR bool i2s_tx_sent_callback(i2s_chan_handle_t handle, i2s_event_data_t* event, void* user_ctx) {

  num_frames = event->size / (TDM_SLOTS * sizeof(int16_t));
  int16_t* dst = (int16_t*)event->dma_buf;

  uint32_t cycles_start = grid_platform_get_cycles();

  double freq_normalized = grid_esp32_codec_state.freq / SAMPLE_RATE;
  double volume_scale = grid_esp32_codec_state.volume;

  for (int i = 0; i < num_frames; i++) {
    double sample_val;
    hexwave_generate_samples(&sample_val, 1, &grid_esp32_codec_state.osc, freq_normalized);

    int16_t sample = (int16_t)(sample_val * volume_scale * 0x7FFF);
    for (int s = 0; s < TDM_SLOTS; s++) {
      dst[i * TDM_SLOTS + s] = sample;
    }
  }

  cycles_elapsed = grid_platform_get_cycles() - cycles_start;
  us_elapsed = cycles_elapsed / grid_platform_get_cycles_per_us();

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
  if (tx_chan == NULL) {
    return;
  }
  ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
  ESP_ERROR_CHECK(i2s_del_channel(tx_chan));
  tx_chan = NULL;
  hexwave_shutdown(NULL);
}

void grid_esp32_codec_init(void) {
  if (tx_chan != NULL) {
    return;
  }
  i2s_example_init_tdm_simplex();

  hexwave_init(12, 4, NULL);
  hexwave_create(&grid_esp32_codec_state.osc, 0, 0.5, 0.0, 0.0);

  grid_esp32_codec_state.freq = FREQ_DEFAULT;
  grid_esp32_codec_state.volume = VOLUME_DEFAULT;

  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void grid_esp32_codec_configure(double freq, double volume, double peak_time, double half_height, double zero_wait) {
  grid_esp32_codec_state.freq = freq;
  grid_esp32_codec_state.volume = volume;
  hexwave_change(&grid_esp32_codec_state.osc, 0, peak_time, half_height, zero_wait);
}

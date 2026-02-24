/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "esp_check.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2s_tdm.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "rom/ets_sys.h"

#include "driver/gptimer.h"
#include "grid_esp32_pins.h"

#include "esp_heap_caps.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "grid_ui_encoder.h"

struct grid_esp32_encoder_model {

  i2s_chan_handle_t rx_chan;
  uint8_t transfer_length;

  grid_process_encoder_t process_encoder;
};

extern struct grid_esp32_encoder_model grid_esp32_encoder_state;

void grid_esp32_encoder_init(struct grid_esp32_encoder_model* encoder, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder);
void grid_esp32_encoder_start(struct grid_esp32_encoder_model* encoder);

#ifdef __cplusplus
}
#endif

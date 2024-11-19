/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_ef44.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

static const char* TAG = "module_ef44";

#define GRID_MODULE_EF44_ENC_NUM 4

static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EF44_ENC_NUM] = {0};

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t* trans) {
  // printf("$\r\n");
}

static void IRAM_ATTR my_post_trans_cb(spi_transaction_t* trans) {

  uint8_t* spi_rx_buffer = &trans->rx_buffer[1]; // SKIP HWCFG BYTE

  struct grid_esp32_encoder_result result = {0};

  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM / 2; i++) {

    result.bytes[i] = spi_rx_buffer[i];
  }

  xRingbufferSendFromISR(grid_esp32_encoder_state.ringbuffer_handle, &result, sizeof(struct grid_esp32_encoder_result), NULL);
}

void grid_esp32_module_ef44_task(void* arg) {
  grid_esp32_encoder_init(&grid_esp32_encoder_state, my_post_setup_cb, my_post_trans_cb);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);
  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EF44_ND_RevD;
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent);
  }

  uint64_t potmeter_last_real_time[4] = {0};
  const uint8_t multiplexer_lookup[4] = {6, 4, 7, 5};
  static const uint8_t invert_result_lookup[4] = {0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 2;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  while (1) {

    size_t adc_result_size = 0;

    struct grid_esp32_adc_result* adc_result;
    adc_result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &adc_result_size, 0);

    if (adc_result != NULL) {

      uint8_t lookup_index = adc_result->mux_state * 2 + adc_result->channel;

      if (invert_result_lookup[lookup_index]) {
        adc_result->value = 4095 - adc_result->value;
      }

      grid_ui_potmeter_store_input(multiplexer_lookup[lookup_index], &potmeter_last_real_time[lookup_index], adc_result->value, 12);
      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, adc_result);
    }

    size_t size = 0;

    struct grid_esp32_encoder_result* result;
    result = (struct grid_esp32_encoder_result*)xRingbufferReceive(grid_esp32_encoder_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t encoder_position_lookup[GRID_MODULE_EF44_ENC_NUM] = {2, 3, 0, 1};

      for (uint8_t j = 0; j < GRID_MODULE_EF44_ENC_NUM; j++) {

        uint8_t new_value = (result->bytes[j / 2] >> (4 * (j % 2))) & 0x0F;

        uint8_t i = encoder_position_lookup[j];

        grid_ui_encoder_store_input(&ui_encoder_state[i], i, new_value);
      }

      vRingbufferReturnItem(grid_esp32_encoder_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

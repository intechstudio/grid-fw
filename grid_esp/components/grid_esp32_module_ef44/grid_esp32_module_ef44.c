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

static uint64_t encoder_last_real_time[16] = {0};
static uint64_t button_last_real_time[16] = {0};
static uint8_t phase_change_lock_array[16] = {0};

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t* trans) {
  // printf("$\r\n");
}

static void IRAM_ATTR my_post_trans_cb(spi_transaction_t* trans) {

  uint8_t* spi_rx_buffer = &trans->rx_buffer[1]; // SKIP HWCFG BYTE

  struct grid_esp32_encoder_result result = {0};

  for (uint8_t i = 0; i < 8; i++) {

    result.bytes[i] = spi_rx_buffer[i];
  }

  xRingbufferSendFromISR(grid_esp32_encoder_state.ringbuffer_handle, &result, sizeof(struct grid_esp32_encoder_result), NULL);

  // grid_esp32_encoder_spi_start_transfer(&grid_esp32_encoder_state);
  // portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
  // portENTER_CRITICAL(&spinlock);
  // spi_device_queue_trans(grid_esp32_encoder_state.spi_device_handle,
  // &grid_esp32_encoder_state.transaction, 0); portEXIT_CRITICAL(&spinlock);
}

void grid_esp32_module_ef44_task(void* arg) {
  grid_esp32_encoder_init(&grid_esp32_encoder_state, my_post_setup_cb, my_post_trans_cb);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);

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

      uint8_t encoder_position_lookup[4] = {2, 3, 0, 1};
      // uint8_t encoder_position_lookup[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12,
      // 13, 8, 9, 4, 5, 0, 1} ;

      // Buffer is only 8 bytes but we check all 16 encoders separately
      for (uint8_t j = 0; j < 4; j++) {

        uint8_t new_value = (result->bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
        uint8_t old_value = grid_esp32_encoder_state.rx_buffer_previous[j];

        grid_esp32_encoder_state.rx_buffer_previous[j] = new_value;

        uint8_t i = encoder_position_lookup[j];

        grid_ui_encoder_store_input(i, &encoder_last_real_time[i], &button_last_real_time[i], old_value, new_value, &phase_change_lock_array[i]);
      }

      vRingbufferReturnItem(grid_esp32_encoder_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

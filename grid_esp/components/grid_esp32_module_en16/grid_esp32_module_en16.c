/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_en16.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_ui_encoder.h"
#include "grid_ui_system.h"

#include "grid_esp32_encoder.h"

static const char* TAG = "module_en16";

#define GRID_MODULE_EN16_ENC_NUM 16

static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EN16_ENC_NUM];

static uint8_t detent;

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t* trans) {
  // printf("$\r\n");
}

static void IRAM_ATTR my_post_trans_cb(spi_transaction_t* trans) {

  uint8_t* spi_rx_buffer = &trans->rx_buffer[1]; // SKIP HWCFG BYTE

  struct grid_esp32_encoder_result result = {0};

  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM / 2; i++) {

    result.bytes[i] = spi_rx_buffer[i];
  }

  xRingbufferSendFromISR(grid_esp32_encoder_state.ringbuffer_handle, &result, sizeof(struct grid_esp32_encoder_result), NULL);
}

void grid_esp32_module_en16_task(void* arg) {
  grid_esp32_encoder_init(&grid_esp32_encoder_state, my_post_setup_cb, my_post_trans_cb);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);
  detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevA && grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevD;

  while (1) {

    size_t size = 0;

    struct grid_esp32_encoder_result* result;
    result = (struct grid_esp32_encoder_result*)xRingbufferReceive(grid_esp32_encoder_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t encoder_position_lookup[GRID_MODULE_EN16_ENC_NUM] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

      // Buffer is only 8 bytes but we check all 16 encoders separately
      for (uint8_t j = 0; j < GRID_MODULE_EN16_ENC_NUM; j++) {

        uint8_t new_value = (result->bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
        uint8_t old_value = grid_esp32_encoder_state.rx_buffer_previous[j];

        grid_esp32_encoder_state.rx_buffer_previous[j] = new_value;

        uint8_t i = encoder_position_lookup[j];

        grid_ui_encoder_store_input(&ui_encoder_state[i], i, old_value, new_value, detent);
      }

      vRingbufferReturnItem(grid_esp32_encoder_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

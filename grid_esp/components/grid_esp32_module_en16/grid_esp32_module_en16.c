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

static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EN16_ENC_NUM] = {0};
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR en16_process_encoder(spi_transaction_t* trans) {

  static uint8_t encoder_lookup[GRID_MODULE_EN16_ENC_NUM] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

  // Skip hwcfg byte
  uint8_t* spi_rx_buffer = &trans->rx_buffer[1];

  struct grid_esp32_encoder_result result = {0};
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM / 2; ++i) {
    result.bytes[i] = spi_rx_buffer[i];
  }

  for (uint8_t j = 0; j < GRID_MODULE_EN16_ENC_NUM; ++j) {

    uint8_t value = (result.bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);
  }
}

void grid_esp32_module_en16_task(void* arg) {

  grid_esp32_encoder_init(&grid_esp32_encoder_state, en16_process_encoder);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);
  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevA && grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevD;
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

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

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_system.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_encoder.h"

// static const char* TAG = "module_en16";

#define GRID_MODULE_EN16_BUT_NUM 16

#define GRID_MODULE_EN16_ENC_NUM 16

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_encoder_state* DRAM_ATTR ui_encoder_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR en16_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EN16_ENC_NUM] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t j = 0; j < GRID_MODULE_EN16_ENC_NUM; ++j) {

    uint8_t value = (bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);

    uint8_t button_value = value & 0b00000100;

    grid_ui_button_store_input(ele, &ui_button_state[idx], button_value, 1);
  }
}

void grid_esp32_module_en16_task(void* arg) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_EN16_BUT_NUM * sizeof(struct grid_ui_button_state));
  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_EN16_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(ui_button_state, 0, GRID_MODULE_EN16_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_encoder_state, 0, GRID_MODULE_EN16_ENC_NUM * sizeof(struct grid_ui_encoder_state));

  for (int i = 0; i < GRID_MODULE_EN16_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  grid_esp32_encoder_init(&grid_esp32_encoder_state, 1, en16_process_encoder);
  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevA && grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevD;
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  GRID_MODULE_DRIVER_INIT_DONE = 1;

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

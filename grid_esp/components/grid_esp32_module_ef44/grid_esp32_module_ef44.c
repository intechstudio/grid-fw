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

// static const char* TAG = "module_ef44";

#define GRID_MODULE_EF44_ENC_NUM 4

#define GRID_MODULE_EF44_POT_NUM 4

static struct grid_ui_encoder_state* DRAM_ATTR ui_encoder_state = NULL;
static uint64_t* DRAM_ATTR potmeter_last_real_time = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR ef44_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {6, 4, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, 10, 8, 11, 9};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (mux_position < 4) {

  } else if (mux_position < 8) {

    grid_ui_potmeter_store_input(ele, mux_position - 4, &potmeter_last_real_time[mux_position - 4], result->value, 12);
  }
}

void IRAM_ATTR ef44_process_encoder(spi_transaction_t* trans) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EF44_ENC_NUM] = {2, 3, 0, 1};

  // Skip hwcfg byte
  uint8_t* spi_rx_buffer = &((uint8_t*)trans->rx_buffer)[1];

  struct grid_esp32_encoder_result result = {0};
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM / 2; ++i) {
    result.bytes[i] = spi_rx_buffer[i];
  }

  for (uint8_t j = 0; j < GRID_MODULE_EF44_ENC_NUM; ++j) {

    uint8_t value = (result.bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);
  }
}

void grid_esp32_module_ef44_task(void* arg) {

  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  potmeter_last_real_time = grid_platform_allocate_volatile(GRID_MODULE_EF44_POT_NUM * sizeof(uint64_t));
  memset(ui_encoder_state, 0, GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(potmeter_last_real_time, 0, GRID_MODULE_EF44_POT_NUM * sizeof(uint64_t));

  grid_esp32_encoder_init(&grid_esp32_encoder_state, ef44_process_encoder);
  grid_esp32_encoder_start(&grid_esp32_encoder_state);
  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EF44_ND_RevD;
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent);
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, ef44_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, 2);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

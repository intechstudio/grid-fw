/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_ef44.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_platform.h"
#include "grid_sys.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

// static const char* TAG = "module_ef44";

#define GRID_MODULE_EF44_ENC_NUM 4

#define GRID_MODULE_EF44_POT_NUM 4

static struct grid_ui_encoder_state* DRAM_ATTR ui_encoder_state = NULL;
static uint64_t* DRAM_ATTR potmeter_last_real_time = NULL;
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR ef44_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {6, 4, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, 10, 8, 11, 9};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];

  if (!grid_asc_process(&asc_state[lookup_index], result->value, &result->value)) {
    return;
  }

  if (mux_position < 4) {

  } else if (mux_position < 8) {

    grid_ui_potmeter_store_input(ele, mux_position - 4, &potmeter_last_real_time[mux_position - 4], result->value, 12);
  }
}

void IRAM_ATTR ef44_process_encoder(void* dma_buf) {

  static DRAM_ATTR uint8_t encoder_lookup[GRID_MODULE_EF44_ENC_NUM] = {2, 3, 0, 1};

  // Skip hwcfg byte
  uint8_t* bytes = &((uint8_t*)dma_buf)[1];

  for (uint8_t j = 0; j < GRID_MODULE_EF44_ENC_NUM; ++j) {

    uint8_t value = (bytes[j / 2] >> (4 * (j % 2))) & 0x0F;
    uint8_t idx = encoder_lookup[j];
    struct grid_ui_element* ele = &elements[idx];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[idx], value);
  }
}

void grid_esp32_module_ef44_task(void* arg) {

  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  potmeter_last_real_time = grid_platform_allocate_volatile(GRID_MODULE_EF44_POT_NUM * sizeof(uint64_t));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_encoder_state, 0, GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(potmeter_last_real_time, 0, GRID_MODULE_EF44_POT_NUM * sizeof(uint64_t));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  grid_esp32_encoder_init(&grid_esp32_encoder_state, 1, ef44_process_encoder);
  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EF44_ND_RevD;
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);

  grid_esp32_adc_init(&grid_esp32_adc_state, ef44_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, 2);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(&grid_sys_state);
  grid_esp32_adc_start(&grid_esp32_adc_state, mux_dependent);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

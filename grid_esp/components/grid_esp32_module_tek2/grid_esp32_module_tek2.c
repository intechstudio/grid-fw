/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_tek2.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_platform.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_endless.h"
#include "grid_ui_system.h"

#include "grid_esp32_adc.h"

// static const char* TAG = "module_tek2";

#define GRID_MODULE_TEK2_POT_NUM 2

#define GRID_MODULE_TEK2_BUT_NUM 10

static struct grid_ui_button_state* DRAM_ATTR ui_button_state = NULL;
static struct grid_ui_endless_state* DRAM_ATTR new_endless_state = NULL;
static struct grid_ui_endless_state* DRAM_ATTR old_endless_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR tek2_process_analog(void* user) {

  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {9, 8, 9, 8, 9, 8, -1, -1, 2, 0, 3, 1, 6, 4, 7, 5};

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t mux_position = multiplexer_lookup[lookup_index];
  struct grid_ui_element* ele = &elements[mux_position];
  uint8_t endless_index = mux_position % 2;

  if (mux_position < 8) {

    grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
  } else if (mux_position < 10) {

    switch (lookup_index) {
    case 0:
    case 1: {
      new_endless_state[endless_index].phase_a = result->value;
    } break;
    case 2:
    case 3: {
      new_endless_state[endless_index].phase_b = result->value;
    } break;
    case 4:
    case 5: {
      new_endless_state[endless_index].button_value = result->value;
      grid_ui_button_store_input(ele, &ui_button_state[mux_position], result->value, 12);
      grid_ui_endless_store_input(ele, mux_position, 12, &new_endless_state[endless_index], &old_endless_state[endless_index]);
    } break;
    }
  }
}

void grid_esp32_module_tek2_task(void* arg) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_BUT_NUM * sizeof(struct grid_ui_button_state));
  new_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  old_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(ui_button_state, 0, GRID_MODULE_TEK2_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(new_endless_state, 0, GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(old_endless_state, 0, GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));

  for (int i = 0; i < GRID_MODULE_TEK2_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state, tek2_process_analog);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(&grid_sys_state);
  grid_esp32_adc_start(&grid_esp32_adc_state, mux_dependent);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

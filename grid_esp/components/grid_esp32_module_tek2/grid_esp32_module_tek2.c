/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_tek2.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_module.h"
#include "grid_platform.h"
#include "grid_sys.h"
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
static struct grid_asc* DRAM_ATTR asc_state = NULL;
static struct grid_ui_element* DRAM_ATTR elements = NULL;

void IRAM_ATTR tek2_process_analog(void* user) {

#define X GRID_MUX_UNUSED
  static DRAM_ATTR const uint8_t multiplexer_lookup[16] = {9, 8, 9, 8, 9, 8, X, X, 2, 0, 3, 1, 6, 4, 7, 5};
#undef X

  assert(user);

  struct grid_esp32_adc_result* result = (struct grid_esp32_adc_result*)user;

  uint8_t lookup_index = result->mux_state * 2 + result->channel;
  uint8_t element_index = multiplexer_lookup[lookup_index];

  assert(element_index != GRID_MUX_UNUSED);
  assert(element_index < GRID_MODULE_TEK2_BUT_NUM);

  struct grid_ui_element* ele = &elements[element_index];
  uint8_t endless_index = element_index % 2;

  if (!grid_asc_process(asc_state, lookup_index, result->value, &result->value)) {
    return;
  }

  if (element_index < GRID_MODULE_TEK2_BUT_NUM - GRID_MODULE_TEK2_POT_NUM) {

    grid_ui_button_store_input(ele, &ui_button_state[element_index], result->value, 12);
  } else if (element_index < GRID_MODULE_TEK2_BUT_NUM) {

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
      grid_ui_button_store_input(ele, &ui_button_state[element_index], result->value, 12);
      grid_ui_endless_store_input(ele, element_index, 12, &new_endless_state[endless_index], &old_endless_state[endless_index]);
    } break;
    }
  }
}

void grid_esp32_module_tek2_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_BUT_NUM * sizeof(struct grid_ui_button_state));
  new_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  old_endless_state = grid_platform_allocate_volatile(GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_TEK2_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(new_endless_state, 0, GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(old_endless_state, 0, GRID_MODULE_TEK2_POT_NUM * sizeof(struct grid_ui_endless_state));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_TEK2_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 8);
  if (grid_hwcfg_module_is_rev_h(sys)) {
    grid_asc_array_set_factors(asc_state, 16, 8, 8, 1);
  }

  elements = grid_ui_model_get_elements(ui);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, 12);

  if (grid_hwcfg_module_is_rev_h(sys)) {

    for (int i = 0; i < 8; ++i) {
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &ui_button_state[i].limits) == 0);
    }

    assert(grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL));
    grid_ui_bulk_flush(ui);
  }

  grid_esp32_adc_init(&grid_esp32_adc_state, tek2_process_analog);
  grid_platform_mux_init(0b11110111);
  uint8_t mux_dependent = !grid_hwcfg_module_is_rev_h(sys);
  grid_esp32_adc_start(&grid_esp32_adc_state, mux_dependent);
}

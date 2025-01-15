/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_tek1.h"

#include <stdint.h>

#include "grid_ain.h"
#include "grid_module.h"
#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_endless.h"
#include "grid_ui_lcd.h"
#include "grid_ui_system.h"

#include "grid_esp32_lcd.h"
#include "grid_font.h"
#include "grid_gui.h"

#include "grid_lua_api.h"

#include "grid_esp32_adc.h"

#include "imgtoc.c"

void imgtoc_rgb888_to_grid_color(uint8_t* src, size_t size, grid_color_t* dest) {

  for (size_t i = 0; i < size; i += 3) {
    dest[i / 3] = ((src[i + 0] << 24) | (src[i + 1] << 16) | (src[i + 2] << 8) | (0xff << 0));
  }
}

static const char* TAG = "module_tek1";

#define GRID_MODULE_TEK1_POT_NUM 2

void grid_esp32_module_tek1_task(void* arg) {

  uint64_t button_last_real_time[15] = {0};

  uint64_t endlesspot_button_last_real_time[2] = {0};
  uint64_t endlesspot_encoder_last_real_time[2] = {0};

  // static const uint8_t multiplexer_lookup[16] = {10, 8, 11, 9, 14, 12, 15,
  // 13, 2, 0, 3, 1, 6, 4, 7, 5};
  static const uint8_t multiplexer_lookup[16] = {9, 8, 11, 10, 13, 12, 15, 14, 2, 0, 3, 1, 6, 4, 7, 5};

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  struct grid_ui_endless_state new_endless_state[GRID_MODULE_TEK1_POT_NUM] = {0};
  struct grid_ui_endless_state old_endless_state[GRID_MODULE_TEK1_POT_NUM] = {0};

  void vsn1_process_analog(void) {
    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(mux_position, &button_last_real_time[mux_position], result->value, 12);

      } else if (mux_position == 9) { // 8, 9

        new_endless_state[0].phase_a = result->value;
      } else if (mux_position == 11) { // 10, 11

        new_endless_state[0].phase_b = result->value;
        // ets_printf("%d \r\n", result->value);
      } else if (mux_position == 13) { // 12, 13

        new_endless_state[0].button_value = result->value;
        grid_ui_button_store_input(8, &old_endless_state[0].button_last_real_time, result->value, 12);
        grid_ui_endless_store_input(8, 12, &new_endless_state[0], &old_endless_state[0]);
      } else if (mux_position == 8 || mux_position == 10 || mux_position == 12 || mux_position == 14) {

        uint8_t btn_num = ((mux_position - 8) / 2) % 4;

        grid_ui_button_store_input(btn_num + 8 + 1, &button_last_real_time[btn_num + 8 + 1], result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }
  void vsn1r_process_analog(void) {
    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(mux_position, &button_last_real_time[mux_position], result->value, 12);

      } else if (mux_position == 8) { // 8, 9

        new_endless_state[0].phase_a = result->value;
      } else if (mux_position == 10) { // 10, 11

        new_endless_state[0].phase_b = result->value;
        // ets_printf("%d \r\n", result->value);
      } else if (mux_position == 12) { // 12, 13

        new_endless_state[0].button_value = result->value;
        grid_ui_button_store_input(8, &old_endless_state[0].button_last_real_time, result->value, 12);
        grid_ui_endless_store_input(8, 12, &new_endless_state[0], &old_endless_state[0]);
      } else if (mux_position == 9 || mux_position == 11 || mux_position == 13 || mux_position == 15) {

        uint8_t btn_num = ((mux_position - 8) / 2) % 4;

        grid_ui_button_store_input(btn_num + 8 + 1, &button_last_real_time[btn_num + 8 + 1], result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }

  void vsn2_process_analog(void) {
    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(mux_position, &button_last_real_time[mux_position], result->value, 12);

      } else if (mux_position == 8 || mux_position == 10 || mux_position == 12 || mux_position == 14) {

        uint8_t btn_num = ((mux_position - 8) / 2) % 4;

        grid_ui_button_store_input(btn_num + 8, &button_last_real_time[btn_num + 8], result->value, 12);
      } else if (mux_position == 9 || mux_position == 11 || mux_position == 13 || mux_position == 15) {

        uint8_t btn_num = ((mux_position - 8 - 1) / 2) % 4;

        grid_ui_button_store_input(btn_num + 8 + 4, &button_last_real_time[btn_num + 8], result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }

  void any_process_analog(void) {

    if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB) {

      vsn1_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB) {

      vsn1r_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {

      vsn2_process_analog();
    }
  }

  // Initialize LCD
  grid_esp32_lcd_model_init(&grid_esp32_lcd_state);

  // Wait for the coprocessor to pull the LCD reset pin high
  vTaskDelay(pdMS_TO_TICKS(500));

  // Initialize LCD at index 0, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_hardware_init(&grid_esp32_lcd_state, 0);
  }

  // Initialize LCD at index 1, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_hardware_init(&grid_esp32_lcd_state, 1);
  }

  // Initialize font
  grid_font_init(&grid_font_state);

  // Initialize GUI
  uint32_t width = 320;
  uint32_t height = 240;
  uint32_t size = width * height * GRID_GUI_BYTES_PPX;
  uint8_t* framebuf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  grid_gui_init(&grid_gui_state, &grid_esp32_lcd_state, framebuf, size, width, height);

  // Allocate transfer buffer
  uint32_t lcd_tx_lines = 4;
  uint32_t lcd_tx_bytes = width * lcd_tx_lines * COLMOD_RGB888_BYTES;
  uint8_t* hw_framebuf = malloc(lcd_tx_bytes);

#define USE_SEMAPHORE
#define USE_FRAMELIMIT

#ifdef USE_FRAMELIMIT
  uint64_t gui_lastrealtime = 0;
#endif

  grid_color_t* matrices[imgtoc_count];

  for (int i = 0; i < imgtoc_count; ++i) {

    matrices[i] = heap_caps_malloc(320 * 240 * 4, MALLOC_CAP_SPIRAM);

    if (!matrices[i]) {
      continue;
    }

    imgtoc_rgb888_to_grid_color(imgtoc[i], 320 * 240 * 3, matrices[i]);
  }

  grid_gui_state.hardwire_matrices = matrices;
  grid_gui_state.hardwire_count = imgtoc_count;

  grid_color_t black = 0x000000ff;
  grid_gui_clear(&grid_gui_state, black);

  uint8_t counter = 0;
  while (1) {

    any_process_analog();

#ifdef USE_FRAMELIMIT
    if (grid_platform_rtc_get_elapsed_time(gui_lastrealtime) < 1000000) {
      taskYIELD();
      continue;
    }
#endif

    ++counter;

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_lock(&grid_lua_state);
#endif

    for (int i = 0; i < grid_gui_state.height; i += lcd_tx_lines) {

      any_process_analog();

      grid_gui_pack_colmod(&grid_gui_state, 0, i, lcd_tx_bytes / COLMOD_RGB888_BYTES, hw_framebuf, COLMOD_RGB888);

      if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA ||
          grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA ||
          grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {

        grid_esp32_lcd_draw_bitmap_blocking(&grid_esp32_lcd_state, 0, 0, i, width, lcd_tx_lines, hw_framebuf);
      }

      if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB ||
          grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
        grid_esp32_lcd_draw_bitmap_blocking(&grid_esp32_lcd_state, 1, 0, i, width, lcd_tx_lines, hw_framebuf);
      }
    }

#ifdef USE_FRAMELIMIT
    gui_lastrealtime = grid_platform_rtc_get_micros();
#endif

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_release(&grid_lua_state);
#endif

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

// || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB)

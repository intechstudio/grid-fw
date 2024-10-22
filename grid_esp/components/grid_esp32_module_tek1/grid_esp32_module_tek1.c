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

static const char* TAG = "module_tek1";

#define COLOR_MODE_1BIT_MONOCHROME

#ifdef COLOR_MODE_24BIT_TRUECOLOR
#define SCREEN_WIDTH 320 / 4
#define SCREEN_HEIGHT 240 / 2
#define FRAMEBUFFER_BYTES_PER_PIXEL 3
#define FRAMEBUFFER_BITS_PER_PIXEL 24
#endif

#ifdef COLOR_MODE_6BIT_RRGGBB
#define SCREEN_WIDTH 320 / 2
#define SCREEN_HEIGHT 240 / 2
#define FRAMEBUFFER_BYTES_PER_PIXEL 1
#define FRAMEBUFFER_BITS_PER_PIXEL 6
#endif

#ifdef COLOR_MODE_1BIT_MONOCHROME
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FRAMEBUFFER_BYTES_PER_PIXEL 1 / 8
#define FRAMEBUFFER_BITS_PER_PIXEL 1
#endif

uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * FRAMEBUFFER_BYTES_PER_PIXEL] = {0};

// This is specific to the ST6678 driver
#define TRANSFERBUFFER_BYTES_PER_PIXEL 3
#define TRANSFERBUFFER_BITS_PER_PIXEL 24
#define TRANSFERBUFFER_LINES 4
uint8_t hw_framebuffer[SCREEN_WIDTH * TRANSFERBUFFER_LINES * TRANSFERBUFFER_BYTES_PER_PIXEL] = {0};

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

  struct grid_module_endless_state current_endlesspot_state[2] = {0};
  struct grid_module_endless_state last_endlesspot_state[2] = {0};

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

        current_endlesspot_state[0].phase_a_value = result->value;
      } else if (mux_position == 11) { // 10, 11

        current_endlesspot_state[0].phase_b_value = result->value;
        // ets_printf("%d \r\n", result->value);
      } else if (mux_position == 13) { // 12, 13

        current_endlesspot_state[0].button_value = result->value;
        grid_ui_button_store_input(8, &endlesspot_button_last_real_time[0], result->value, 12);
        grid_ui_endless_store_input(8, &endlesspot_encoder_last_real_time[0], &last_endlesspot_state[0], &current_endlesspot_state[0], 12);
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

        current_endlesspot_state[0].phase_a_value = result->value;
      } else if (mux_position == 10) { // 10, 11

        current_endlesspot_state[0].phase_b_value = result->value;
        // ets_printf("%d \r\n", result->value);
      } else if (mux_position == 12) { // 12, 13

        current_endlesspot_state[0].button_value = result->value;
        grid_ui_button_store_input(8, &endlesspot_button_last_real_time[0], result->value, 12);
        grid_ui_endless_store_input(8, &endlesspot_encoder_last_real_time[0], &last_endlesspot_state[0], &current_endlesspot_state[0], 12);
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

    if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA) {

      vsn1_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA) {

      vsn1r_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {

      vsn2_process_analog();
    }
  }

  grid_esp32_lcd_model_init(&grid_esp32_lcd_state);

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_hardware_init(&grid_esp32_lcd_state, 0);
  }

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_hardware_init(&grid_esp32_lcd_state, 1);
  }

  grid_font_init(&grid_font_state);
  grid_gui_init(&grid_gui_state, &grid_esp32_lcd_state, framebuffer, sizeof(framebuffer), FRAMEBUFFER_BITS_PER_PIXEL, SCREEN_WIDTH, SCREEN_HEIGHT);

  uint8_t loopcounter = 0;

  uint64_t gui_lastrealtime = 0;
  struct grid_gui_model* gui = &grid_gui_state;

  // grid_gui_draw_demo(&grid_gui_state, loopcounter);
  grid_gui_draw_clear(&grid_gui_state);

  while (1) {

#define USE_SEMAPHORE
#define USE_FRAMELIMIT

    // DO GUI THINGS
    loopcounter++;
    grid_gui_draw_demo(&grid_gui_state, loopcounter);

    // memset(framebuffer, 255, sizeof(framebuffer));
    any_process_analog();
    if (grid_gui_state.framebuffer_changed_flag == 0) {
      taskYIELD();
      continue;
    }

#ifdef USE_FRAMELIMIT
    if (grid_platform_rtc_get_elapsed_time(gui_lastrealtime) < 30000) {
      taskYIELD();
      continue;
    }
#endif

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_lock(&grid_lua_state);
#endif

    gui_lastrealtime = grid_platform_rtc_get_micros();

    grid_gui_state.framebuffer_changed_flag = 0;

    for (int i = 0; i < SCREEN_HEIGHT; i += TRANSFERBUFFER_LINES) {

      any_process_analog();

      if (FRAMEBUFFER_BITS_PER_PIXEL == 24 && TRANSFERBUFFER_BITS_PER_PIXEL == 24) {
        memcpy(hw_framebuffer, gui->framebuffer + i * SCREEN_WIDTH * 3, (SCREEN_WIDTH * TRANSFERBUFFER_LINES * TRANSFERBUFFER_BYTES_PER_PIXEL));
      } else if (FRAMEBUFFER_BITS_PER_PIXEL == 6 && TRANSFERBUFFER_BITS_PER_PIXEL == 24) {

        for (int y = i; y < i + TRANSFERBUFFER_LINES; y++) {
          for (int x = 0; x < gui->width; x++) {

            uint32_t index_in_buffer = (y * gui->width + x) * 1;
            if (x == y) {
              gui->framebuffer[index_in_buffer] = 255;
            }

            uint32_t index_out_buffer = ((y - i) * gui->width + x) * TRANSFERBUFFER_BYTES_PER_PIXEL;
            hw_framebuffer[index_out_buffer] = ((gui->framebuffer[index_in_buffer] >> 4) & 0b00000011) * 85;
            hw_framebuffer[index_out_buffer + 1] = ((gui->framebuffer[index_in_buffer] >> 2) & 0b00000011) * 85;
            hw_framebuffer[index_out_buffer + 2] = ((gui->framebuffer[index_in_buffer] >> 0) & 0b00000011) * 85;

            // hw_framebuffer[index_out_buffer+1] = 255;
          }
        }
      } else if (gui->bits_per_pixel == 1) {
        for (int y = i; y < i + TRANSFERBUFFER_LINES; y++) {
          for (int x = 0; x < gui->width; x++) {

            uint32_t index_in_buffer = (y * gui->width + x) / 8;
            uint32_t offset_in_buffer = (y * gui->width + x) % 8;

            uint32_t index_out_buffer = ((y - i) * gui->width + x) * TRANSFERBUFFER_BYTES_PER_PIXEL;

            uint8_t intensity = ((gui->framebuffer[index_in_buffer] >> (offset_in_buffer)) & 0b00000001) * 255;

            hw_framebuffer[index_out_buffer + 0] = intensity * grid_sys_state.bank_activebank_color_r / 255;
            hw_framebuffer[index_out_buffer + 1] = intensity * grid_sys_state.bank_activebank_color_g / 255;
            hw_framebuffer[index_out_buffer + 2] = intensity * grid_sys_state.bank_activebank_color_b / 255;

            // hw_framebuffer[index_out_buffer+1] = 255;
          }
        }
      } else {
        abort();
      }

      if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA ||
          grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {

        grid_esp32_lcd_draw_bitmap_blocking(&grid_esp32_lcd_state, 0, 0, i, SCREEN_WIDTH, TRANSFERBUFFER_LINES, hw_framebuffer);
      }

      if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA ||
          grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
        grid_esp32_lcd_draw_bitmap_blocking(&grid_esp32_lcd_state, 1, 0, i, SCREEN_WIDTH, TRANSFERBUFFER_LINES, hw_framebuffer);
      }
    }

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_release(&grid_lua_state);
#endif

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

// || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB)

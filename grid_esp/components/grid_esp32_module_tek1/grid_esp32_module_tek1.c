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

uint16_t vmp_get_scanline() {

    uint16_t scan;
    grid_esp32_lcd_get_scanline(&grid_esp32_lcd_state, 0, 14, &scan);
    return scan;
}

#include "vmp_def.h"
#include "vmp_tag.h"

#include "imgtoc.c"

void imgtoc_rgb888_to_grid_color(uint8_t* src, size_t size, grid_color_t* dest) {

  for (size_t i = 0; i < size; i += 3) {
    dest[i / 3] = ((src[i + 0] << 24) | (src[i + 1] << 16) | (src[i + 2] << 8) | (0xff << 0));
  }
}

uint8_t* hw_colbufs[3] = {0};
uint8_t counter = 0;

void grid_esp32_module_vsn_lcd_refresh(struct grid_esp32_lcd_model* lcd, int lines, int tx_lines, int ready_len) {

  bool active[2] = {
    grid_esp32_lcd_panel_active(lcd, 0),
    grid_esp32_lcd_panel_active(lcd, 1),
  };

  bool done[2] = { !active[0], !active[1] };

  int index = 2;

  uint16_t scan;

  int columns = grid_gui_state.height;

  while (!(done[0] && done[1])) {

    if (index >= 2) {

      uint16_t scans[2];
      bool ready[2] = { false, false };
      for (int i = 0; i < 2; ++i) {

        if (!done[i]) {

          grid_esp32_lcd_get_scanline(lcd, i, 14, &scans[i]);
          ready[i] = grid_esp32_lcd_scan_in_range(lines + 1, tx_lines, ready_len, scans[i]);
        }
      }

      if (ready[0] && ready[1]) {
        index = scans[0] < scans[1] ? 1 : 0;
      } else
      if (ready[0] || ready[1]) {
        index = ready[0] ? 0 : 1;
      }

    } else {

      uint8_t rates[3] = { 0x1d, 0x1e, 0x1f };
      for (int i = 0; i < lines; i += tx_lines) {

        grid_esp32_lcd_get_scanline(lcd, index, 14, &scan);
        while (grid_esp32_lcd_scan_in_range(lines + 1, i, tx_lines, scan)) {
          grid_esp32_lcd_get_scanline(lcd, index, 14, &scan);
        }

        uint8_t* buf = hw_colbufs[counter % 3];
        grid_esp32_lcd_draw_bitmap_blocking(lcd, index, i, 0, tx_lines, columns / 1, buf);
      }

      done[index] = true;

      index = 2;
    }
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
  grid_esp32_lcd_spi_bus_init(&grid_esp32_lcd_state);

  // Wait for the coprocessor to pull the LCD reset pin high
  vTaskDelay(pdMS_TO_TICKS(500));

  // Initialize LCD panel at index 0, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&grid_esp32_lcd_state, 0);
    grid_esp32_lcd_set_frctrl2(&grid_esp32_lcd_state, 0, 0x1e);
  }

  // Initialize LCD panel at index 1, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&grid_esp32_lcd_state, 1);
    grid_esp32_lcd_set_frctrl2(&grid_esp32_lcd_state, 1, 0x1e);
  }

  // Initialize font
  grid_font_init(&grid_font_state);

  // Initialize GUI
  uint32_t width = 320;
  uint32_t height = 240;
  uint32_t lines = width;
  uint32_t size = width * height * GRID_GUI_BYTES_PPX;
  uint8_t* framebuf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  grid_gui_init(&grid_gui_state, &grid_esp32_lcd_state, framebuf, size, width, height);

  // Allocate transfer buffer
  uint32_t lcd_tx_lines = 8;
  uint32_t lcd_tx_bytes = height * lcd_tx_lines * COLMOD_RGB888_BYTES;

  hw_colbufs[0] = heap_caps_malloc(lcd_tx_bytes, MALLOC_CAP_SPIRAM);
  hw_colbufs[1] = heap_caps_malloc(lcd_tx_bytes, MALLOC_CAP_SPIRAM);
  hw_colbufs[2] = heap_caps_malloc(lcd_tx_bytes, MALLOC_CAP_SPIRAM);
  grid_color_t cols[3] = { 0xff0000ff, 0x00ff00ff, 0x0000ffff };
  for (int i = 0; i < 3; ++i) {

      grid_platform_printf("hw_colbufs[%d]: %p\n", i, hw_colbufs[i]);
      for (int j = 0; j < lcd_tx_bytes; j += COLMOD_RGB888_BYTES) {
        hw_colbufs[i][j + 0] = (cols[i] >> 24) & 0xff;
        hw_colbufs[i][j + 1] = (cols[i] >> 16) & 0xff;
        hw_colbufs[i][j + 2] = (cols[i] >> 8) & 0xff;
      }
  }

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

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };

  bool vmp_flushed = false;
  struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_state;
  while (1) {

    //vmp_push(MAIN);

    if (!vmp_flushed && vmp.size == vmp.capacity) {

      portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
      portENTER_CRITICAL(&spinlock);

      vmp_serialize_start(&reg);
      vmp_buf_serialize_and_write(&vmp, &reg);
      vmp_uid_str_serialize_and_write(VMP_UID_COUNT, VMP_ASSOC, &reg);
      vmp_serialize_close(&reg);

      portEXIT_CRITICAL(&spinlock);

      //vmp_buf_free(&vmp);

      vmp_flushed = true;
    }

    any_process_analog();

    ++counter;

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_lock(&grid_lua_state);
#endif

    grid_esp32_module_vsn_lcd_refresh(lcd, lines, lcd_tx_lines, width / 4);

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_release(&grid_lua_state);
#endif

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

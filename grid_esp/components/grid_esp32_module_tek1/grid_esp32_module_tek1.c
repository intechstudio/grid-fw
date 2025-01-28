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

void imgtoc_rgb888_to_grid_color(uint8_t* src, size_t size, size_t w, size_t h, grid_color_t* dest) {

  for (size_t x = 0; x < w; ++x) {
    for (size_t y = 0; y < h; ++y) {
      uint8_t* pixel = &src[(y * w + x) * COLMOD_RGB888_BYTES];
      dest[h * x + y] = (pixel[0] << 24) | (pixel[1] << 16) | (pixel[2] << 8) | (0xff << 0);
    }
  }
}

int grid_esp32_module_vsn_lcd_wait_scan_top(struct grid_esp32_lcd_model* lcd, bool active[2], int lines, int tx_lines, int ready_len, uint16_t scans[2]) {

  // Whether the scanline is in the allowable range
  bool ready[2] = {false, false};

  // Wait for an active panel's scanline
  while (true) {

    // Check scanlines for the active panels
    for (int i = 0; i < 2; ++i) {

      if (!active[i]) {
        continue;
      }

      grid_esp32_lcd_get_scanline(lcd, i, LCD_SCAN_OFFSET, &scans[i]);

      // A panel is ready when its scanline enters the allowable range
      ready[i] = grid_esp32_lcd_scan_in_range(lines + 1, tx_lines, ready_len, scans[i]);
    }

    // If both panels are ready
    if (ready[0] && ready[1]) {

      // Choose the panel whose scanline has progressed further
      return scans[0] < scans[1] ? 1 : 0;

    } else
      // If only a single panel is ready
      if (ready[0] || ready[1]) {

        // Choose that panel
        return ready[0] ? 0 : 1;
      }
  }
}

void grid_esp32_module_vsn_lcd_sync_before_push(struct grid_esp32_lcd_model* lcd, uint8_t lcd_index, int lines, bool scans[2]) {

  bool active[2] = {
      grid_esp32_lcd_panel_active(lcd, 0),
      grid_esp32_lcd_panel_active(lcd, 1),
  };

  if (!(active[0] && active[1])) {
    return;
  }

  // At the moment, synchronize only before the first panel is pushed
  if (lcd_index == 0) {

    // Whether the other scanline is in the next half cycle
    int half = LCD_SCAN_VALUES / 2;
    bool currhalf = grid_esp32_lcd_scan_in_range(lines + 1, scans[0], half, scans[1]);

    // Frequency control byte
    uint8_t frctrl = currhalf ? LCD_FRCTRL_39HZ : LCD_FRCTRL_41HZ;

    // Slow down the second panel when it is in the current half cycle,
    // speed it up when it is in the next half cycle, with the aim of
    // keeping the two scanlines synchronized
    grid_esp32_lcd_set_frctrl2(lcd, 1, frctrl);
  }
}

void grid_esp32_module_vsn_lcd_push_trailing(struct grid_esp32_lcd_model* lcd, int lcd_index, int lines, int columns, int tx_lines, uint8_t* frame, uint8_t* xferbuf) {

  uint16_t scan;

  // Transfer all lines, n lines at a time
  for (int i = 0; i < lines; i += tx_lines) {

    int row = (lcd_index == 0) ? i : lines - tx_lines - i;

    // Copy n lines into a transfer buffer
    uint8_t* src = &frame[row * columns * COLMOD_RGB888_BYTES];
    memcpy(xferbuf, src, tx_lines * columns * COLMOD_RGB888_BYTES);

    // Wait while the scanline is in the region to be transferred,
    // to make sure our writes are always trailing it
    grid_esp32_lcd_get_scanline(lcd, lcd_index, LCD_SCAN_OFFSET, &scan);
    while (grid_esp32_lcd_scan_in_range(lines + 1, i, tx_lines, scan)) {
      grid_esp32_lcd_get_scanline(lcd, lcd_index, LCD_SCAN_OFFSET, &scan);
    }

    // Transfer n lines
    grid_esp32_lcd_draw_bitmap_blocking(lcd, lcd_index, 0, row, columns / 1, tx_lines, xferbuf);
  }
}

void grid_esp32_module_vsn_lcd_refresh(struct grid_esp32_lcd_model* lcd, struct grid_gui_model* guis, int lines, int columns, int tx_lines, int ready_len, uint8_t* xferbuf) {

  bool waiting[2] = {
      grid_esp32_lcd_panel_active(lcd, 0),
      grid_esp32_lcd_panel_active(lcd, 1),
  };

  while (waiting[0] || waiting[1]) {

    uint16_t scans[2];

    int lcd_index = grid_esp32_module_vsn_lcd_wait_scan_top(lcd, waiting, lines, tx_lines, ready_len, scans);

    // grid_esp32_module_vsn_lcd_sync_before_push(lcd, lcd_index, lines, scans);

    vmp_push(TOP);
    grid_esp32_module_vsn_lcd_push_trailing(lcd, lcd_index, lines, columns, tx_lines, guis[lcd_index].buffer, xferbuf);
    vmp_push(BOT);

    waiting[lcd_index] = false;
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

  struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_state;

  // Allocate transfer buffer
  uint32_t width = LCD_HRES;
  uint32_t height = LCD_VRES;
  uint32_t lcd_tx_lines = 16;
  uint32_t lcd_tx_bytes = height * lcd_tx_lines * COLMOD_RGB888_BYTES;
  uint8_t* xferbuf = malloc(lcd_tx_bytes);

  // Initialize LCD
  grid_esp32_lcd_spi_bus_init(&grid_esp32_lcd_state, lcd_tx_bytes);

  // Wait for the coprocessor to pull the LCD reset pin high
  vTaskDelay(pdMS_TO_TICKS(500));

  // Initialize LCD panel at index 0, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&grid_esp32_lcd_state, 0);
    grid_esp32_lcd_set_frctrl2(&grid_esp32_lcd_state, 0, LCD_FRCTRL_40HZ);
  }

  // Initialize LCD panel at index 1, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB) {
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(&grid_esp32_lcd_state, 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(&grid_esp32_lcd_state, 1);
    grid_esp32_lcd_set_frctrl2(&grid_esp32_lcd_state, 1, LCD_FRCTRL_40HZ);
  }

  // Initialize font
  grid_font_init(&grid_font_state);

  uint32_t hwcfg = grid_sys_get_hwcfg(&grid_sys_state);

  // Initialize GUIs
  uint32_t lines = width;
  uint32_t columns = height;
  uint32_t size = width * height * GRID_GUI_BYTES_PPX;

  struct grid_gui_model* guis = grid_gui_states;

  // Initialize GUI at index 0, if necessary
  if (hwcfg == GRID_MODULE_TEK1_RevA || hwcfg == GRID_MODULE_VSN1_RevA || hwcfg == GRID_MODULE_VSN1_RevB || hwcfg == GRID_MODULE_VSN2_RevA || hwcfg == GRID_MODULE_VSN2_RevB) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[0], &grid_esp32_lcd_state, buf, size, width, height);
  }

  // Initialize GUI panel at index 1, if necessary
  if (hwcfg == GRID_MODULE_TEK1_RevA || hwcfg == GRID_MODULE_VSN1_RevA || hwcfg == GRID_MODULE_VSN1_RevB || hwcfg == GRID_MODULE_VSN2_RevA || hwcfg == GRID_MODULE_VSN2_RevB) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[1], &grid_esp32_lcd_state, buf, size, width, height);
  }

  grid_color_t* matrices[imgtoc_count];

  for (int i = 0; i < imgtoc_count; ++i) {

    matrices[i] = heap_caps_malloc(320 * 240 * 4, MALLOC_CAP_SPIRAM);

    if (!matrices[i]) {
      continue;
    }

    imgtoc_rgb888_to_grid_color(imgtoc[i], 320 * 240 * 3, 320, 240, matrices[i]);
  }

  for (int i = 0; i < 2; ++i) {
    grid_gui_states[i].hardwire_matrices = matrices;
    grid_gui_states[i].hardwire_count = imgtoc_count;
  }

#define USE_SEMAPHORE
#define USE_FRAMELIMIT

#ifdef USE_FRAMELIMIT
  uint64_t gui_lastrealtime = 0;
#endif

  for (int i = 0; i < 2; ++i) {
    grid_color_t clear = grid_gui_color_from_rgb(0, 0, 0);
    grid_gui_clear(&guis[i], clear);
  }

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };

  uint8_t counter = 0;

  bool vmp_flushed = false;
  while (1) {

    // vmp_push(MAIN);

    if (!vmp_flushed && vmp.size == vmp.capacity) {

      portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
      portENTER_CRITICAL(&spinlock);

      vmp_serialize_start(&reg);
      vmp_buf_serialize_and_write(&vmp, &reg);
      vmp_uid_str_serialize_and_write(VMP_UID_COUNT, VMP_ASSOC, &reg);
      vmp_serialize_close(&reg);

      portEXIT_CRITICAL(&spinlock);

      // vmp_buf_free(&vmp);

      vmp_flushed = true;
    }

    any_process_analog();

    // vmp_push(TOP);
    grid_gui_draw_demo(&guis[0], counter);
    // grid_gui_draw_demo_image(&guis[0], 0);
    // grid_gui_draw_demo_rgb(&guis[0], counter);
    // vmp_push(BOT);
    grid_gui_draw_demo(&guis[1], 255 - counter);
    // grid_gui_draw_demo_image(&guis[1], 0);
    // grid_gui_draw_demo_rgb(&guis[1], counter);

    ++counter;

    /*
    if (counter >= SZ_COLBUFS) {
      counter = 0;
    }
    */

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_lock(&grid_lua_state);
#endif

    grid_esp32_module_vsn_lcd_refresh(lcd, guis, lines, columns, lcd_tx_lines, width / 16, xferbuf);

#ifdef USE_SEMAPHORE
    grid_lua_semaphore_release(&grid_lua_state);
#endif

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

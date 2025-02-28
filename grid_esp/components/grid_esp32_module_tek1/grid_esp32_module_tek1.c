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

#include "vmp_def.h"
#include "vmp_tag.h"

// static const char* TAG = "module_tek1";

#define GRID_MODULE_TEK1_BUT_NUM 17

#define GRID_MODULE_TEK1_POT_NUM 2

void grid_esp32_module_tek1_task(void* arg) {

  static struct grid_ui_button_state ui_button_state[GRID_MODULE_TEK1_BUT_NUM] = {0};

  for (int i = 0; i < GRID_MODULE_TEK1_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 12, 0.5, 0.2);
  }

  // uint64_t endlesspot_button_last_real_time[2] = {0};
  // uint64_t endlesspot_encoder_last_real_time[2] = {0};

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  struct grid_ui_endless_state new_endless_state[GRID_MODULE_TEK1_POT_NUM] = {0};
  struct grid_ui_endless_state old_endless_state[GRID_MODULE_TEK1_POT_NUM] = {0};

  void vsn1_process_analog(void) {

    static const uint8_t multiplexer_lookup[16] = {8, 9, 8, 10, 8, 11, -1, 12, 2, 0, 3, 1, 6, 4, 7, 5};

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);

      } else if (mux_position < 9) {

        switch (lookup_index) {
        case 0: {

          new_endless_state[0].phase_a = result->value;
        } break;
        case 2: {

          new_endless_state[0].phase_b = result->value;
        } break;
        case 4: {

          new_endless_state[0].button_value = result->value;
          grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);
          grid_ui_endless_store_input(mux_position, 12, &new_endless_state[0], &old_endless_state[0]);
        } break;
        }

      } else if (mux_position < 13) {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }

  void vsn1r_process_analog(void) {

    static const uint8_t multiplexer_lookup[16] = {9, 8, 10, 8, 11, 8, 12, -1, 2, 0, 3, 1, 6, 4, 7, 5};

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);

      } else if (mux_position < 9) {

        switch (lookup_index) {
        case 1: {

          new_endless_state[0].phase_a = result->value;
        } break;
        case 3: {

          new_endless_state[0].phase_b = result->value;
        } break;
        case 5: {

          new_endless_state[0].button_value = result->value;
          grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);
          grid_ui_endless_store_input(mux_position, 12, &new_endless_state[0], &old_endless_state[0]);
        } break;
        }

      } else if (mux_position < 13) {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }

  void vsn2_process_analog(void) {

    static const uint8_t multiplexer_lookup[16] = {13, 8, 14, 9, 15, 10, 16, 11, 2, 0, 3, 1, 6, 4, 7, 5};

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;
      uint8_t mux_position = multiplexer_lookup[lookup_index];

      if (mux_position < 8) {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);

      } else {

        grid_ui_button_store_input(&ui_button_state[mux_position], mux_position, result->value, 12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }
  }

  void any_process_analog(void) {

    if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevA ||
        grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevH) {

      vsn1_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB ||
               grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevH) {

      vsn1r_process_analog();
    } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB ||
               grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevH) {

      vsn2_process_analog();
    }
  }

  struct grid_esp32_lcd_model* lcds = grid_esp32_lcd_states;

  // Allocate transfer buffer
  uint32_t width = LCD_HRES;
  uint32_t height = LCD_VRES;
  uint32_t lcd_tx_lines = 16;
  uint32_t lcd_tx_bytes = height * lcd_tx_lines * COLMOD_RGB888_BYTES;
  // uint8_t* xferbuf = malloc(lcd_tx_bytes);

  // Initialize LCD
  grid_esp32_lcd_spi_bus_init(lcd_tx_bytes);

  // Wait for the coprocessor to pull the LCD reset pin high
  vTaskDelay(pdMS_TO_TICKS(500));

  // Initialize LCD panel at index 0, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1L_RevH || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevH) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[0];
    grid_esp32_lcd_panel_init(lcd, 0, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, 0, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Initialize LCD panel at index 1, if necessary
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1R_RevH ||
      grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevH) {

    struct grid_esp32_lcd_model* lcd = &grid_esp32_lcd_states[1];
    grid_esp32_lcd_panel_init(lcd, 1, GRID_LCD_CLK_SLOW);
    grid_esp32_lcd_panel_init(lcd, 1, GRID_LCD_CLK_FAST);
    grid_esp32_lcd_panel_reset(lcd);
    grid_esp32_lcd_panel_set_frctrl2(lcd, LCD_FRCTRL_40HZ);
  }

  // Initialize font
  grid_font_init(&grid_font_state);

  uint32_t hwcfg = grid_sys_get_hwcfg(&grid_sys_state);

  // Initialize GUIs
  // uint32_t lines = width;
  // uint32_t columns = height;
  uint32_t size = width * height * GRID_GUI_BYTES_PPX;

  struct grid_gui_model* guis = grid_gui_states;

  // Initialize GUI at index 0, if necessary
  if (hwcfg == GRID_MODULE_TEK1_RevA || hwcfg == GRID_MODULE_VSN1L_RevA || hwcfg == GRID_MODULE_VSN1L_RevB || hwcfg == GRID_MODULE_VSN1L_RevH || hwcfg == GRID_MODULE_VSN2_RevA ||
      hwcfg == GRID_MODULE_VSN2_RevB || hwcfg == GRID_MODULE_VSN2_RevH) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[0], &lcds[0], buf, size, width, height);
  }

  // Initialize GUI panel at index 1, if necessary
  if (hwcfg == GRID_MODULE_VSN1R_RevA || hwcfg == GRID_MODULE_VSN1R_RevB || hwcfg == GRID_MODULE_VSN1R_RevH || hwcfg == GRID_MODULE_VSN2_RevA || hwcfg == GRID_MODULE_VSN2_RevB ||
      hwcfg == GRID_MODULE_VSN2_RevH) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[1], &lcds[1], buf, size, width, height);
  }

  // Clear active panels
  for (int i = 0; i < 2; ++i) {

    if (grid_esp32_lcd_panel_active(&lcds[i])) {

      grid_color_t clear = grid_gui_color_from_rgb(0, 0, 0);
      grid_gui_clear(&guis[i], clear);
      grid_gui_swap_set(&guis[i], true);
    }
  }

  // Mark the LCD as ready
  grid_esp32_lcd_set_ready(true);

#undef USE_SEMAPHORE
#undef USE_FRAMELIMIT

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };

  // uint8_t counter = 0;

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

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);

  (void)VMP_ALLOC;
  (void)VMP_DEALLOC;
}

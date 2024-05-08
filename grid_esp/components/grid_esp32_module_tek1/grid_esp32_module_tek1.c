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
#include "grid_ui_system.h"

#include "grid_esp32_lcd.h"
#include "grid_font.h"
#include "grid_gui.h"

#include "grid_esp32_adc.h"

static const char* TAG = "module_tek1";

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define FRAMEBUFFER_BYTES_PER_PIXEL 1
#define FRAMEBUFFER_BITS_PER_PIXEL 6
uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * FRAMEBUFFER_BYTES_PER_PIXEL] = {0};

#define TRANSFERBUFFER_BYTES_PER_PIXEL 3
#define TRANSFERBUFFER_BITS_PER_PIXEL 24
#define TRANSFERBUFFER_LINES 4
uint8_t hw_framebuffer[SCREEN_WIDTH * TRANSFERBUFFER_LINES * TRANSFERBUFFER_BYTES_PER_PIXEL] = {0};

void grid_esp32_module_tek1_task(void* arg) {

  uint64_t button_last_real_time[8] = {0};

  uint64_t endlesspot_button_last_real_time[2] = {0};
  uint64_t endlesspot_encoder_last_real_time[2] = {0};

  // static const uint8_t multiplexer_lookup[16] = {10, 8, 11, 9, 14, 12, 15,
  // 13, 2, 0, 3, 1, 6, 4, 7, 5};
  static const uint8_t multiplexer_lookup[16] = {9, 8, 11, 10, 13, 12, -1, -1, 2, 0, 3, 1, 6, 4, 7, 5};

  // static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0, 0, 0, 0, 0, 0, 0};
  const uint8_t multiplexer_overflow = 8;

  grid_esp32_adc_init(&grid_esp32_adc_state);
  grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
  grid_esp32_adc_start(&grid_esp32_adc_state);

  struct grid_module_endless_state current_endlesspot_state[2] = {0};
  struct grid_module_endless_state last_endlesspot_state[2] = {0};

  grid_esp32_lcd_model_init(&grid_esp32_lcd_state);
  grid_esp32_lcd_hardware_init(&grid_esp32_lcd_state);
  grid_font_init(&grid_font_state);
  grid_gui_init(&grid_gui_state, &grid_esp32_lcd_state, framebuffer, sizeof(framebuffer), FRAMEBUFFER_BITS_PER_PIXEL, SCREEN_WIDTH, SCREEN_HEIGHT);

  uint8_t loopcounter = 0;

  while (1) {

    loopcounter++;
    struct grid_gui_model* gui = &grid_gui_state;

    grid_gui_draw_demo(&grid_gui_state, loopcounter);

    // memset(framebuffer, 255, sizeof(framebuffer));

    for (int i = 0; i < SCREEN_HEIGHT; i += TRANSFERBUFFER_LINES) {

      if (FRAMEBUFFER_BITS_PER_PIXEL == 24 && TRANSFERBUFFER_BITS_PER_PIXEL == 24) {
        memcpy(hw_framebuffer, gui->framebuffer + i * SCREEN_WIDTH * FRAMEBUFFER_BYTES_PER_PIXEL, (SCREEN_WIDTH * TRANSFERBUFFER_LINES * TRANSFERBUFFER_BYTES_PER_PIXEL));
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
      } else {
        abort();
      }

      grid_esp32_lcd_draw_bitmap_blocking(&grid_esp32_lcd_state, 0, i, SCREEN_WIDTH, TRANSFERBUFFER_LINES, hw_framebuffer);
    }

    // ESP_LOGI(TAG, "Loop2: %d", loopcounter++);
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  while (1) {

    size_t size = 0;

    struct grid_esp32_adc_result* result;
    result = (struct grid_esp32_adc_result*)xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle, &size, 0);

    if (result != NULL) {

      uint8_t lookup_index = result->mux_state * 2 + result->channel;

      if (multiplexer_lookup[lookup_index] < 8) {

        grid_ui_button_store_input(multiplexer_lookup[lookup_index], &button_last_real_time[multiplexer_lookup[lookup_index]], result->value, 12);
      } else if (multiplexer_lookup[lookup_index] < 10) { // 8, 9

        uint8_t endlesspot_index = multiplexer_lookup[lookup_index] % 2;
        current_endlesspot_state[endlesspot_index].phase_a_value = result->value;
      } else if (multiplexer_lookup[lookup_index] < 12) { // 10, 11

        uint8_t endlesspot_index = multiplexer_lookup[lookup_index] % 2;
        current_endlesspot_state[endlesspot_index].phase_b_value = result->value;
        // ets_printf("%d \r\n", result->value);
      } else if (multiplexer_lookup[lookup_index] < 14) { // 12, 13

        uint8_t endlesspot_index = multiplexer_lookup[lookup_index] % 2;
        current_endlesspot_state[endlesspot_index].button_value = result->value;
        grid_ui_button_store_input(8 + endlesspot_index, &endlesspot_button_last_real_time[endlesspot_index], result->value, 12);

        grid_ui_endless_store_input(8 + endlesspot_index, &endlesspot_encoder_last_real_time[endlesspot_index], &last_endlesspot_state[endlesspot_index], &current_endlesspot_state[endlesspot_index],
                                    12);
      }

      vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle, result);
    }

    taskYIELD();
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

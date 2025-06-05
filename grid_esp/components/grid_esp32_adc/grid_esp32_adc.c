/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_adc.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"

#include "rom/ets_sys.h"

#include "grid_esp32_pins.h"

#include "esp_heap_caps.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ulp_grid_esp32_adc.h"
#include "ulp_riscv.h"
#include "ulp_riscv_adc.h"

#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "esp_private/rtc_ctrl.h"

void* grid_platform_allocate_volatile(size_t size);

extern const uint8_t ulp_grid_esp32_adc_bin_start[] asm("_binary_ulp_grid_esp32_adc_bin_start");
extern const uint8_t ulp_grid_esp32_adc_bin_end[] asm("_binary_ulp_grid_esp32_adc_bin_end");

struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;

void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow) {

  gpio_set_direction(GRID_ESP32_PINS_MUX_0_A, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_0_B, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_0_C, GPIO_MODE_OUTPUT);

  gpio_set_direction(GRID_ESP32_PINS_MUX_1_A, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_1_B, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_1_C, GPIO_MODE_OUTPUT);

  adc->mux_index = 0;

  grid_esp32_adc_mux_update(adc);

  adc->mux_overflow = mux_overflow;
  ulp_mux_overflow = adc->mux_overflow;
}

void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc) { adc->mux_index = (adc->mux_index + 1) % adc->mux_overflow; }

void IRAM_ATTR grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc) {

  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_A, adc->mux_index >> 0 & 0x1);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_B, adc->mux_index >> 1 & 0x1);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_C, adc->mux_index >> 2 & 0x1);

  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_A, adc->mux_index >> 0 & 0x1);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_B, adc->mux_index >> 1 & 0x1);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_C, adc->mux_index >> 2 & 0x1);
}

#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_private/esp_sleep_internal.h"
#include "hal/adc_hal_common.h"

static esp_err_t ulp_riscv_adc_init2(void) {

  const ulp_adc_cfg_t cfg = {
      .adc_n = ADC_UNIT_1,
      .channel = ADC_CHANNEL_0,
      .width = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,
      .ulp_mode = ADC_ULP_MODE_RISCV,
  };

  return ulp_adc_init(&cfg);
}

static void adc_init_ulp(struct grid_esp32_adc_model* adc) {

  ESP_ERROR_CHECK(ulp_riscv_adc_init2());

  // Load ULP-RISC-V program binary into RTC memory
  const uint8_t* binary = ulp_grid_esp32_adc_bin_start;
  size_t size = ulp_grid_esp32_adc_bin_end - binary;
  ESP_ERROR_CHECK(ulp_riscv_load_binary(binary, size));
}

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, grid_process_analog_t process_analog) {

  assert(process_analog);

  adc->process_analog = process_analog;

  adc_init_ulp(adc);
}

static void IRAM_ATTR ulp_isr(void* arg) {

  struct grid_esp32_adc_model* adc = (struct grid_esp32_adc_model*)arg;

  if (adc->mux_dependent) {
    grid_esp32_adc_conv_mux();
  } else {
    grid_esp32_adc_conv_nomux();
  }
}

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc, uint8_t mux_dependent) {

  // Set flag for both processors indicating which one does mux addressing
  adc->mux_dependent = mux_dependent != 0;
  ulp_mux_dependent = adc->mux_dependent;

  // Register ISR handler
  ulp_riscv_isr_register(ulp_isr, adc, ULP_RISCV_SW_INT);

  // Configure the ULP with defaults and run the program loaded into RTC memory
  ESP_ERROR_CHECK(ulp_riscv_run());
}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc) { assert(0); }

void IRAM_ATTR grid_esp32_adc_conv_mux() {

  struct grid_esp32_adc_model* adc = &grid_esp32_adc_state;

  if (!adc->process_analog) {
    return;
  }

  uint8_t mux_state = adc->mux_index;
  grid_esp32_adc_mux_increment(adc);
  grid_esp32_adc_mux_update(adc);

  uint32_t adc_value[2];
  memcpy(adc_value, &ulp_adc_value, sizeof(adc_value));

  ulp_adc_result_taken = 1;

  for (int i = 0; i < 2; ++i) {

    struct grid_esp32_adc_result result;
    result.channel = i;
    result.mux_state = mux_state;
    result.value = adc_value[i];

    adc->process_analog(&result);
  }

  ulp_adc_result_ready = 0;
}

void IRAM_ATTR grid_esp32_adc_conv_nomux() {

  struct grid_esp32_adc_model* adc = &grid_esp32_adc_state;

  if (!adc->process_analog) {
    return;
  }

  uint32_t adc_value[8][2];
  memcpy(adc_value, &ulp_adc_value, sizeof(adc_value));

  ulp_adc_result_taken = 1;

  for (int i = 0; i < 8; ++i) {

    for (int j = 0; j < 2; ++j) {

      struct grid_esp32_adc_result result;
      result.channel = j;
      result.mux_state = i;
      result.value = adc_value[i][j];

      adc->process_analog(&result);
    }
  }

  ulp_adc_result_ready = 0;
}

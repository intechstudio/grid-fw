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

  const char* TAG = "ulp_riscv_adc2";
  esp_err_t ret = ESP_OK;

  const ulp_riscv_adc_cfg_t cfg = {
      .adc_n = ADC_UNIT_1,
      .channel = ADC_CHANNEL_1,
      .width = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,
  };

  ESP_GOTO_ON_FALSE(cfg.adc_n == ADC_UNIT_1, ESP_ERR_INVALID_ARG, err, TAG, "Only ADC_UNIT_1 is supported for now");

  // Initialize ADC1
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = cfg.adc_n,
      .ulp_mode = ADC_ULP_MODE_RISCV,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  // Configure ADC1
  adc_oneshot_chan_cfg_t config = {
      .bitwidth = cfg.width,
      .atten = cfg.atten,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config));

  // Calibrate the ADC
  adc_set_hw_calibration_code(cfg.adc_n, cfg.atten);
  esp_sleep_enable_adc_tsens_monitor(true);

err:
  return ret;
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

#include "rom/ets_sys.h"

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc, uint8_t mux_dependent) {

  // Set flag for both processors indicating which one does mux addressing
  adc->mux_dependent = mux_dependent != 0;
  ulp_mux_dependent = adc->mux_dependent;

  // Register bit storing the status of the ULP-RISCV interrupt
  uint32_t bit = RTC_CNTL_COCPU_INT_ST_M;

  // Register a handler for a specific RTC_CNTL interrupt
  ESP_ERROR_CHECK(rtc_isr_register(ulp_isr, adc, bit, RTC_INTR_FLAG_IRAM));

  REG_SET_BIT(RTC_CNTL_INT_ENA_REG, bit);

  // Configure the ULP with defaults and run the program loaded into RTC memory
  ESP_ERROR_CHECK(ulp_riscv_run());
}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc) { assert(0); }

#include "esp_cpu.h"

void IRAM_ATTR grid_esp32_adc_conv_mux() {

  struct grid_esp32_adc_model* adc = &grid_esp32_adc_state;

  if (!adc->process_analog) {
    return;
  }

  if (ulp_adc_result_ready < ulp_adc_oversample) {
    return;
  }

  uint8_t mux_state = adc->mux_index;
  grid_esp32_adc_mux_increment(adc);
  grid_esp32_adc_mux_update(adc);

  uint32_t adc_value[2] = {ulp_adc_value_0, ulp_adc_value_1};

  for (int i = 0; i < 2; ++i) {

    struct grid_esp32_adc_result result;
    result.channel = i;
    result.mux_state = mux_state;
    result.value = adc_value[i];

    adc->process_analog(&result);
  }

  ulp_sum_value_0 = 0;
  ulp_sum_value_1 = 0;
  ulp_adc_result_ready = 0;

  if (adc->mux_index == 7) {
    static uint32_t cycles_prev = 0;
    uint32_t cycles = esp_cpu_get_cycle_count();
    ets_printf("clocks %d\n", cycles - cycles_prev);
    cycles_prev = cycles;
  }
}

void IRAM_ATTR grid_esp32_adc_conv_nomux() {}

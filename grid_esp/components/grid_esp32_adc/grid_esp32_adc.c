/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_adc.h"

#include "esp_check.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_timer.h"

#include "rom/ets_sys.h" // For ets_printf

#include "grid_esp32_pins.h"

#include "esp_heap_caps.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "ulp_grid_esp32_adc.h"
#include "ulp_riscv.h"
#include "ulp_riscv_adc.h"

#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

extern const uint8_t ulp_grid_esp32_adc_bin_start[] asm("_binary_ulp_grid_esp32_adc_bin_start");
extern const uint8_t ulp_grid_esp32_adc_bin_end[] asm("_binary_ulp_grid_esp32_adc_bin_end");

static void init_ulp_program(void);

extern uint32_t ulp_adc_value_1;
extern uint32_t ulp_adc_value_2;

extern uint32_t ulp_adc_result_ready;

extern uint32_t ulp_lock;

struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;

void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow) {

  gpio_set_direction(GRID_ESP32_PINS_MUX_0_A, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_0_B, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_0_C, GPIO_MODE_OUTPUT);

  gpio_set_direction(GRID_ESP32_PINS_MUX_1_A, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_1_B, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_MUX_1_C, GPIO_MODE_OUTPUT);

  gpio_set_level(GRID_ESP32_PINS_MUX_0_A, 0);
  gpio_set_level(GRID_ESP32_PINS_MUX_0_B, 0);
  gpio_set_level(GRID_ESP32_PINS_MUX_0_C, 0);

  gpio_set_level(GRID_ESP32_PINS_MUX_1_A, 0);
  gpio_set_level(GRID_ESP32_PINS_MUX_1_B, 0);
  gpio_set_level(GRID_ESP32_PINS_MUX_1_C, 0);

  adc->mux_overflow = mux_overflow;
}

void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc) {
  adc->mux_index++;
  adc->mux_index %= adc->mux_overflow;
}

void IRAM_ATTR grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc) {

  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_A, adc->mux_index / 1 % 2);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_B, adc->mux_index / 2 % 2);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_C, adc->mux_index / 4 % 2);

  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_A, adc->mux_index / 1 % 2);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_B, adc->mux_index / 2 % 2);
  gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_C, adc->mux_index / 4 % 2);
}

uint8_t IRAM_ATTR grid_esp32_adc_mux_get_index(struct grid_esp32_adc_model* adc) { return adc->mux_index; }

static void init_ulp_program(void) {
  esp_err_t err = ulp_riscv_load_binary(ulp_grid_esp32_adc_bin_start, (ulp_grid_esp32_adc_bin_end - ulp_grid_esp32_adc_bin_start));
  ESP_ERROR_CHECK(err);

  /* The first argument is the period index, which is not used by the ULP-RISC-V
   * timer The second argument is the period in microseconds, which gives a
   * wakeup time period of: 20ms
   */
  ulp_set_wakeup_period(0, 1);

  /* Start the program */
  // err = ulp_riscv_run();
  // ESP_ERROR_CHECK(err);
}

#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_private/esp_sleep_internal.h"
#include "hal/adc_hal_common.h"

static esp_err_t ulp_riscv_adc_init2(void) {

  ulp_riscv_adc_cfg_t cfg_in = {
      .adc_n = ADC_UNIT_1,
      .channel = ADC_CHANNEL_1,
      .width = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_11,
  };

  const ulp_riscv_adc_cfg_t* cfg = &cfg_in;

  esp_err_t ret = ESP_OK;

  const char* TAG = "ulp_riscv_adc2";

  ESP_GOTO_ON_FALSE(cfg, ESP_ERR_INVALID_ARG, err, TAG, "cfg == NULL");
  ESP_GOTO_ON_FALSE(cfg->adc_n == ADC_UNIT_1, ESP_ERR_INVALID_ARG, err, TAG, "Only ADC_UNIT_1 is supported for now");

  //-------------ADC1 Init---------------//
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = cfg->adc_n,
      .ulp_mode = ADC_ULP_MODE_RISCV,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  //-------------ADC1 Config---------------//
  adc_oneshot_chan_cfg_t config = {
      .bitwidth = cfg->width,
      .atten = cfg->atten,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config));

  // Calibrate the ADC
  adc_set_hw_calibration_code(cfg->adc_n, cfg->atten);
  esp_sleep_enable_adc_tsens_monitor(true);

err:
  return ret;
}

static void adc_init(struct grid_esp32_adc_model* adc) {

  ESP_ERROR_CHECK(ulp_riscv_adc_init2());

  init_ulp_program();
}

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, SemaphoreHandle_t nvm_semaphore) {

  adc->nvm_semaphore = nvm_semaphore;

  adc->buffer_struct = (StaticRingbuffer_t*)heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  adc->buffer_storage = (struct grid_esp32_adc_result*)heap_caps_malloc(sizeof(struct grid_esp32_adc_result) * ADC_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  adc->ringbuffer_handle = xRingbufferCreateStatic(ADC_BUFFER_SIZE, ADC_BUFFER_TYPE, adc->buffer_storage, adc->buffer_struct);

  adc_init(adc);

  adc->mux_index = 0;
}

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc) {

  //  start periodic task

  esp_timer_create_args_t periodic_adc_args = {.callback = &grid_esp32_adc_convert, .name = "adc millisecond"};

  esp_timer_handle_t periodic_adc_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_adc_args, &periodic_adc_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_adc_timer, 1000));

  esp_err_t err = ulp_riscv_run();
  ESP_ERROR_CHECK(err);
}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc) {}

#include "ulp_riscv_lock.h"

static float restrictToRange(float value) {
  if (value < 0.0) {
    return 0.0;
  } else if (value > 1.0) {
    return 1.0;
  } else {
    return value;
  }
}

static uint32_t grid_esp32_adc_cal(uint32_t input) {

  // parameter 1: center offset
  int32_t parameter_1 = +32 * 4.5;
  uint32_t ADC_MAX = ((1 << 12) - 1);

  // (-abs(x-128)+96)/64
  // normalized: (-abs(x-0.5)+0.375)/0.25
  // (-abs(x-0.5))*2.4+ 1.1

  float strength = restrictToRange((-abs(input - 0.5 * ADC_MAX)) * 2.4 / ADC_MAX + 1.1);

  return input + parameter_1 * strength;
}

void IRAM_ATTR grid_esp32_adc_convert(void) {

  struct grid_esp32_adc_model* adc = &grid_esp32_adc_state;

  // if (xSemaphoreTakeFromISR(adc->nvm_semaphore, NULL) == pdTRUE){

  ulp_riscv_lock_t* lock = (ulp_riscv_lock_t*)&ulp_lock;

  ulp_riscv_lock_acquire(lock);

  if (ulp_adc_result_ready > 0 && ulp_adc_result_ready < UINT32_MAX / 2) {

    struct grid_esp32_adc_result result_0;
    result_0.channel = 0;
    result_0.mux_state = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);
    result_0.value = grid_esp32_adc_cal(ulp_adc_value_1);

    struct grid_esp32_adc_result result_1;
    result_1.channel = 1;
    result_1.mux_state = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);
    result_1.value = grid_esp32_adc_cal(ulp_adc_value_2);

    xRingbufferSendFromISR(adc->ringbuffer_handle, &result_0, sizeof(struct grid_esp32_adc_result), NULL);
    xRingbufferSendFromISR(adc->ringbuffer_handle, &result_1, sizeof(struct grid_esp32_adc_result), NULL);

    // ets_printf("%d\r\n", ulp_adc_result_ready);

    ulp_adc_result_ready = UINT32_MAX; // start new conversion

    grid_esp32_adc_mux_increment(&grid_esp32_adc_state);
    grid_esp32_adc_mux_update(&grid_esp32_adc_state);
  } else {
  }

  ulp_riscv_lock_release(lock);

  //     xSemaphoreGiveFromISR(adc->nvm_semaphore, NULL);
  // }
}

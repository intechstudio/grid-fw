/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "ulp_riscv.h"
#include "ulp_riscv_print.h"
#include "ulp_riscv_utils.h"

#include "sdkconfig.h"
#include "ulp_riscv_adc_ulp_core.h"
#include "ulp_riscv_gpio.h"
#include "ulp_riscv_uart_ulp_core.h"

#include "hal/adc_types.h"

#include "grid_esp32_pins.h"

// volatile, to avoid being optimized away
volatile uint32_t mux_logic_activated = 0;
volatile uint32_t mux_overflow = 8;
volatile uint32_t mux_index = 0;

void grid_ulp_adc_mux_init(void) {
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_C);

  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_C);
}

void grid_ulp_adc_mux_increment(void) { mux_index = (mux_index + 1) % mux_overflow; }

void grid_ulp_adc_mux_update(void) {
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_A, mux_index / 1 % 2);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_B, mux_index / 2 % 2);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_C, mux_index / 4 % 2);
}

volatile uint32_t adc_oversample = 8;
volatile uint32_t adc_result_ready = 0;
volatile uint32_t adc_value_0 = 0;
volatile uint32_t adc_value_1 = 0;
volatile uint32_t sum_value_0 = 0;
volatile uint32_t sum_value_1 = 0;

uint32_t ADC_CHANNELS[2] = {
    ADC_CHANNEL_1,
    ADC_CHANNEL_0,
};

int main(void) {

  if (mux_logic_activated) {
    grid_ulp_adc_mux_init();
  }

  while (1) {

    while (adc_result_ready >= adc_oversample) {
      continue;
    }
    if (mux_logic_activated && adc_result_ready == 0) { // just quit the busy loop
      grid_ulp_adc_mux_increment();
      grid_ulp_adc_mux_update();
    }

    uint32_t value[2] = {0};
    value[0] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[0]);
    value[1] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[1]);

    if (value[0] == -1 || value[1] == -1) {
      continue;
    }

    sum_value_0 += value[0];
    sum_value_1 += value[1];

    adc_value_0 = sum_value_0 / (adc_result_ready + 1);
    adc_value_1 = sum_value_1 / (adc_result_ready + 1);

    ++adc_result_ready;
  }
}

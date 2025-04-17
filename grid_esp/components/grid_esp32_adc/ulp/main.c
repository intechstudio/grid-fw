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
#include "ulp_riscv_uart_ulp_core.h"

#include "hal/adc_types.h"

#include "grid_esp32_pins.h"

const uint32_t ADC_CHANNELS[2] = {ADC_CHANNEL_1, ADC_CHANNEL_0};

// volatile, to avoid being optimized away
volatile uint32_t mux_dependent = 0;
volatile uint32_t mux_overflow = 0;
volatile uint32_t mux_index = 0;
volatile uint32_t adc_result_ready = 0;
volatile uint32_t adc_result_taken = 1;
volatile uint32_t adc_value[8][2] = {0};

void grid_ulp_adc_mux_init(void) {

  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_C);

  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_C);
}

static inline void grid_ulp_adc_mux_increment() { mux_index = (mux_index + 1) % mux_overflow; }

static inline void grid_ulp_adc_mux_update() {

  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_A, mux_index & 0x1);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_B, mux_index & 0x2);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_C, mux_index & 0x4);
}

static inline uint32_t adc_read_with_mux(adc_unit_t adc_n, volatile uint32_t ret[2]) {

  const uint32_t event = (adc_n == ADC_UNIT_1) ? ADC_LL_EVENT_ADC1_ONESHOT_DONE : ADC_LL_EVENT_ADC2_ONESHOT_DONE;

  adc_oneshot_ll_clear_event(event);
  adc_oneshot_ll_disable_all_unit();
  adc_oneshot_ll_enable(adc_n);
  adc_oneshot_ll_set_channel(adc_n, ADC_CHANNELS[0]);
  adc_oneshot_ll_start(adc_n);

  // Increment mux here, while the conversion is running
  grid_ulp_adc_mux_increment();

  while (adc_oneshot_ll_get_event(event) != true) {
    ;
  }
  ret[0] = adc_oneshot_ll_get_raw_result(adc_n);
  adc_oneshot_ll_disable_all_unit();

  adc_oneshot_ll_clear_event(event);
  adc_oneshot_ll_disable_all_unit();
  adc_oneshot_ll_enable(adc_n);
  adc_oneshot_ll_set_channel(adc_n, ADC_CHANNELS[1]);
  adc_oneshot_ll_start(adc_n);
  while (adc_oneshot_ll_get_event(event) != true) {
    ;
  }

  // Update mux pins here, as early as possible
  grid_ulp_adc_mux_update();

  ret[1] = adc_oneshot_ll_get_raw_result(adc_n);
  adc_oneshot_ll_disable_all_unit();
}

int main(void) {

  if (!mux_dependent) {
    grid_ulp_adc_mux_init();
  }

  while (1) {

    if (!adc_result_taken) {
      continue;
    }

    if (mux_dependent) {

      adc_value[0][0] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[0]);
      adc_value[0][1] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[1]);

    } else {

      for (int i = 0; i < mux_overflow; ++i) {

        adc_read_with_mux(ADC_UNIT_1, adc_value[i]);
      }
    }

    while (adc_result_ready) {
      ;
    }

    adc_result_taken = 0;
    adc_result_ready = 1;

    ulp_riscv_wakeup_main_processor();
  }
}

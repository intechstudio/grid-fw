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

#include "grid_ain.h"
#include "grid_esp32_pins.h"

const uint32_t ADC_CHANNELS[2] = {ADC_CHANNEL_1, ADC_CHANNEL_0};

// volatile, to avoid being optimized away
volatile uint32_t mux_dependent = 0;
volatile uint32_t mux_index = 0;
volatile uint32_t mux_positions_bm = 0xFF;
volatile uint32_t adc_result_ready = 0;
volatile uint32_t adc_result_taken = 1;
volatile uint32_t adc_value[8][2] = {0};

void grid_platform_mux_write(uint8_t index) {
  mux_index = index;
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_A, mux_index / 1 % 2);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_B, mux_index / 2 % 2);
  ulp_riscv_gpio_output_level(GRID_ESP32_PINS_MUX_1_C, mux_index / 4 % 2);
}

void grid_platform_mux_init(uint8_t positions_bm) {
  mux_positions_bm = positions_bm;

  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_init(GRID_ESP32_PINS_MUX_1_C);

  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_A);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_B);
  ulp_riscv_gpio_output_enable(GRID_ESP32_PINS_MUX_1_C);

  GRID_MUX_FIRST_VALID(mux_index, mux_positions_bm);

  grid_platform_mux_write(mux_index);
}

static inline uint32_t adc_read_with_mux(adc_unit_t adc_n, volatile uint32_t ret[2]) {

  const uint32_t event = (adc_n == ADC_UNIT_1) ? ADC_LL_EVENT_ADC1_ONESHOT_DONE : ADC_LL_EVENT_ADC2_ONESHOT_DONE;

  adc_oneshot_ll_clear_event(event);
  adc_oneshot_ll_disable_all_unit();
  adc_oneshot_ll_enable(adc_n);
  adc_oneshot_ll_set_channel(adc_n, ADC_CHANNELS[0]);
  adc_oneshot_ll_start(adc_n);

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

  // Advance to next mux position
  GRID_MUX_INCREMENT(mux_index, mux_positions_bm);
  grid_platform_mux_write(mux_index);

  ret[1] = adc_oneshot_ll_get_raw_result(adc_n);
  adc_oneshot_ll_disable_all_unit();
}

int main(void) {

  if (!mux_dependent) {
    grid_platform_mux_init(mux_positions_bm);
  }

  while (1) {

    if (!adc_result_taken) {
      continue;
    }

    if (mux_dependent) {

      adc_value[0][0] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[0]);
      adc_value[0][1] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[1]);

    } else {

      uint32_t start_pos = mux_index;
      do {
        uint32_t pos = mux_index;
        adc_read_with_mux(ADC_UNIT_1, adc_value[pos]);
      } while (mux_index != start_pos);
    }

    while (adc_result_ready) {
      ;
    }

    adc_result_taken = 0;
    adc_result_ready = 1;

    ulp_riscv_wakeup_main_processor();
  }
}

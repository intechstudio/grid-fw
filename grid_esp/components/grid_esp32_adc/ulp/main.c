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

// volatile, to avoid being optimized away
volatile uint32_t adc_oversample = 8;
volatile uint32_t adc_result_ready = 0;
volatile uint32_t adc_value[2] = {0};
volatile uint32_t sum_value[2] = {0};

uint32_t ADC_CHANNELS[2] = {
    ADC_CHANNEL_1,
    ADC_CHANNEL_0,
};

int main(void) {

  while (1) {

    if (adc_result_ready >= adc_oversample) {
      continue;
    }

    uint32_t value[2] = {0};
    value[0] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[0]);
    value[1] = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNELS[1]);

    if (!(value[0] != -1 && value[1] != -1)) {
      continue;
    }

    sum_value[0] += value[0];
    sum_value[1] += value[1];

    adc_value[0] = sum_value[0] / (adc_result_ready + 1);
    adc_value[1] = sum_value[1] / (adc_result_ready + 1);

    ++adc_result_ready;
  }
}

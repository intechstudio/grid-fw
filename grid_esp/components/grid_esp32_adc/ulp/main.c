/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP-RISC-V example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This code runs on ULP-RISC-V  coprocessor
*/

#include "ulp_riscv.h"
#include "ulp_riscv_print.h"
#include "ulp_riscv_utils.h"

#include "sdkconfig.h"
#include "ulp_riscv_adc_ulp_core.h"
#include "ulp_riscv_uart_ulp_core.h"

#include "hal/adc_types.h"

#include "ulp_riscv_lock_ulp_core.h"
ulp_riscv_lock_t lock;

#define EXAMPLE_ADC_CHANNEL ADC_CHANNEL_0
#define EXAMPLE_ADC_UNIT ADC_UNIT_1
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_11
#define EXAMPLE_ADC_WIDTH ADC_BITWIDTH_DEFAULT

/* Set high threshold, approx. 1.75V*/
#define EXAMPLE_ADC_TRESHOLD 2000

#define SAMPLE_COUNT 16

static ulp_riscv_uart_t s_print_uart;

uint32_t adc_value_1 = 0;
uint32_t adc_value_2 = 0;

volatile uint32_t adc_result_ready = 0; // volatile otherwise compiler optimizes away external access
volatile uint32_t ready_state = 0;

uint32_t sum_1 = 0;
uint32_t sum_2 = 0;

int main(void) {
  ulp_riscv_uart_cfg_t cfg = {
      .tx_pin = 16,
  };

  ulp_riscv_uart_init(&s_print_uart, &cfg);
  ulp_riscv_print_install((putc_fn_t)ulp_riscv_uart_putc, &s_print_uart);

  int cnt = 0;

  while (1) {

    uint32_t value_1 = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNEL_1);
    uint32_t value_2 = ulp_riscv_adc_read_channel(ADC_UNIT_1, ADC_CHANNEL_0);

    ulp_riscv_lock_acquire(&lock);

    if (value_1 != -1 && value_2 != -1) {

      sum_1 += value_1;
      sum_2 += value_2;
    }

    if (adc_result_ready < UINT32_MAX / 2) {

      ulp_riscv_gpio_output_level(cfg.tx_pin, 0);

      adc_result_ready++;
      adc_value_1 = sum_1 / adc_result_ready;
      adc_value_2 = sum_2 / adc_result_ready;
    } else { // result was read during the conversion, drop the latest sample

      ulp_riscv_gpio_output_level(cfg.tx_pin, 1);

      adc_result_ready++;
      sum_1 = 0;
      sum_2 = 0;
    }

    ulp_riscv_lock_release(&lock);

    // ulp_riscv_delay_cycles(1 * ULP_RISCV_CYCLES_PER_MS);
  }
}

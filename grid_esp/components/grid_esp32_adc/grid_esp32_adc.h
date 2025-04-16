/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef GRID_ESP32_ADC_H
#define GRID_ESP32_ADC_H

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#include "driver/gptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRID_ESP32_ADC_PROCESS_TASK_DELAY_MS 2

#define ADC_SAMPLE_COUNT 16
// #define ADC_CONVERSION_FRAME_SIZE ADC_SAMPLE_COUNT* SOC_ADC_DIGI_DATA_BYTES_PER_CONV
// #define ADC_BUFFER_SIZE ADC_CONVERSION_FRAME_SIZE

#define ADC_BUFFER_SIZE 4 * 25 // 32-bit aligned size
#define ADC_BUFFER_TYPE RINGBUF_TYPE_NOSPLIT

#define ADC_TIMER_PERIOD_USEC 1000

typedef void (*grid_process_analog_t)(void* user);

struct grid_esp32_adc_model {

  uint8_t mux_index;
  uint8_t mux_overflow;
  uint8_t mux_dependent;

  grid_process_analog_t process_analog;
};

struct grid_esp32_adc_result {

  uint8_t channel;
  uint8_t mux_state;
  uint16_t value;
};

extern struct grid_esp32_adc_model grid_esp32_adc_state;

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, grid_process_analog_t process_analog);

void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow);

void grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc);
void grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc);

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc, uint8_t mux_dependent);
void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc);

void grid_esp32_adc_conv_mux();
void grid_esp32_adc_conv_nomux();

#ifdef __cplusplus
}
#endif

#endif /* GRID_ESP32_ADC_H */

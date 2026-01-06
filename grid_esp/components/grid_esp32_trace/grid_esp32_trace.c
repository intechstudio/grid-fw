/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_trace.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <stdint.h>
#include <string.h>

#include "grid_platform.h"

DRAM_ATTR struct grid_esp32_trace_model grid_esp32_trace_core0 = {0};
DRAM_ATTR struct grid_esp32_trace_model grid_esp32_trace_core1 = {0};

void IRAM_ATTR grid_esp32_trace_timed_task_set(struct grid_esp32_trace_model* trace, void* task) { trace->timed_task = task; }

void IRAM_ATTR grid_esp32_trace_timed_task_begin(struct grid_esp32_trace_model* trace) {

  trace->timed_task_last = grid_platform_rtc_get_micros();
  trace->timed_task_micros = 0;
}

uint64_t IRAM_ATTR grid_esp32_trace_timed_task_diff(struct grid_esp32_trace_model* trace) {

  uint64_t now = grid_platform_rtc_get_micros();
  trace->timed_task_micros += grid_platform_rtc_get_diff(now, trace->timed_task_last);

  uint64_t ret = trace->timed_task_micros;

  grid_esp32_trace_timed_task_begin(trace);

  return ret;
}

void IRAM_ATTR grid_esp32_trace_task_switched_in(void) {

  const void* current_task = xTaskGetCurrentTaskHandle();
  const uint8_t core = xTaskGetCoreID((void*)current_task);
  struct grid_esp32_trace_model* trace = core ? &grid_esp32_trace_core1 : &grid_esp32_trace_core0;

  if (current_task == trace->timed_task) {

    trace->timed_task_last = grid_platform_rtc_get_micros();
  }
}

void IRAM_ATTR grid_esp32_trace_task_switched_out(void) {

  const void* current_task = xTaskGetCurrentTaskHandle();
  const uint8_t core = xTaskGetCoreID((void*)current_task);
  struct grid_esp32_trace_model* trace = core ? &grid_esp32_trace_core1 : &grid_esp32_trace_core0;

  if (current_task == trace->timed_task) {

    uint64_t now = grid_platform_rtc_get_micros();
    trace->timed_task_micros += grid_platform_rtc_get_diff(now, trace->timed_task_last);
  }
}

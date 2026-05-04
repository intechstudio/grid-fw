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

DRAM_ATTR struct grid_esp32_trace_model grid_esp32_trace_state = {0};

void IRAM_ATTR grid_esp32_trace_timed_task_set(struct grid_esp32_trace_model* trace, void* task) { trace->timed_task = task; }

inline void IRAM_ATTR grid_esp32_trace_accumulate(struct grid_esp32_trace_model* trace) {

  uint64_t now = grid_platform_rtc_get_micros();
  trace->timed_task_micros += grid_platform_rtc_get_diff(now, trace->timed_task_last);
}

inline void IRAM_ATTR grid_esp32_trace_resume(struct grid_esp32_trace_model* trace) { trace->timed_task_last = grid_platform_rtc_get_micros(); }

void IRAM_ATTR grid_esp32_trace_timed_task_begin(struct grid_esp32_trace_model* trace) {

  trace->timed_task_micros = 0;

  grid_esp32_trace_resume(trace);
}

uint64_t IRAM_ATTR grid_esp32_trace_timed_task_diff(struct grid_esp32_trace_model* trace) {

  grid_esp32_trace_accumulate(trace);

  return trace->timed_task_micros;
}

void IRAM_ATTR grid_esp32_trace_task_switched_in(void) {

  struct grid_esp32_trace_model* trace = &grid_esp32_trace_state;

  const void* current_task = xTaskGetCurrentTaskHandle();

  if (current_task == trace->timed_task) {

    grid_esp32_trace_resume(trace);
  }
}

void IRAM_ATTR grid_esp32_trace_task_switched_out(void) {

  struct grid_esp32_trace_model* trace = &grid_esp32_trace_state;

  const void* current_task = xTaskGetCurrentTaskHandle();

  if (current_task == trace->timed_task) {

    grid_esp32_trace_accumulate(trace);
  }
}

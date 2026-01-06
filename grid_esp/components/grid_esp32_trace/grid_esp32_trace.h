/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "rom/ets_sys.h"

struct grid_esp32_trace_model {

  void* timed_task;
  uint64_t timed_task_last;
  uint64_t timed_task_micros;
};

extern struct grid_esp32_trace_model grid_esp32_trace_core0;

void grid_esp32_trace_timed_task_set(struct grid_esp32_trace_model* trace, void* task);
void grid_esp32_trace_timed_task_begin(struct grid_esp32_trace_model* trace);
uint64_t grid_esp32_trace_timed_task_diff(struct grid_esp32_trace_model* trace);

void grid_esp32_trace_task_switched_in(void);
void grid_esp32_trace_task_switched_out(void);

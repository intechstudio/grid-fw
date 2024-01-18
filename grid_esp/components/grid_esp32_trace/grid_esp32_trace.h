/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <stdio.h> // Include necessary headers if needed

#include "rom/ets_sys.h" // For ets_printf

struct grid_trace_event {

  uint32_t timestamp;
  uint32_t event_type;
  uint32_t event_context;
  uint32_t event_value;
};

struct grid_trace_model {

  uint8_t core_id;
  uint32_t switch_in_count;
  uint32_t switch_out_count;
  uint32_t last_active_handle;
  uint32_t idle_handle;
  void* ignored_task;

  uint32_t trace_buffer_write_ptr;
  uint32_t trace_buffer_read_ptr;
  struct grid_trace_event trace_buffer[1000];
};

extern struct grid_trace_model grid_trace_state_core0;
extern struct grid_trace_model grid_trace_state_core1;

void grid_trace_init(struct grid_trace_model* trace, uint8_t core_id);

void grid_trace_ignore_task(struct grid_trace_model* trace, void* task_handle);

void grid_trace_task_switched_in(void);
void grid_trace_task_switched_out(void);

void grid_trace_report_task(void* arg);

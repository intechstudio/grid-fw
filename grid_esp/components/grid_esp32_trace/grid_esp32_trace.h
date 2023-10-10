/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdio.h> // Include necessary headers if needed
#include <stdint.h>


#include "rom/ets_sys.h" // For ets_printf


struct grid_trace_model{

    uint8_t switch_count;
};


extern struct grid_trace_model grid_trace_state;

void grid_trace_init(struct grid_trace_model* trace);

void grid_trace_task_switched_in(void);

void grid_trace_report_task(void *arg);

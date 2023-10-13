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

    uint8_t core_id;
    uint32_t switch_in_count;
    uint32_t switch_out_count;
    uint32_t last_active_handle;
    uint32_t idle_handle;

    uint32_t timestamps[1000];


};


extern struct grid_trace_model grid_trace_state_core0;
extern struct grid_trace_model grid_trace_state_core1;

void grid_trace_init(struct grid_trace_model* trace, uint8_t core_id);

void grid_trace_task_switched_in(void);
void grid_trace_task_switched_out(void);

void grid_trace_report_task(void *arg);

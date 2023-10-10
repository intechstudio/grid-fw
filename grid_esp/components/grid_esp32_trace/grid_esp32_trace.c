/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_trace.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdint.h>

struct grid_trace_model grid_trace_state;

void grid_trace_init(struct grid_trace_model* trace){


    trace->switch_count = 0;

}


void grid_trace_task_switched_in(void)
{
    struct grid_trace_model* trace = &grid_trace_state;

    trace->switch_count++;

    //

}


void grid_trace_report_task(void *arg)
{

    #ifndef gridUSE_TRACE
        vTaskSuspend(NULL);
        return;
    #endif

    grid_trace_init(&grid_trace_state);

    char stats[3000] = {0};


    while(1){
        ets_printf("Switch: %d\r\n", grid_trace_state.switch_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //Wait to be deleted
    vTaskSuspend(NULL);
}


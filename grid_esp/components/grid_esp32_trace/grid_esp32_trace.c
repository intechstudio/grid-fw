/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_trace.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" // For ets_printf

#include <stdint.h>

struct grid_trace_model grid_trace_state_core0;
struct grid_trace_model grid_trace_state_core1;

void grid_trace_init(struct grid_trace_model* trace, uint8_t core_id){

    trace->core_id = core_id;

    trace->switch_in_count = 0;
    trace->switch_out_count = 0;

    trace->last_active_handle = 0;
    trace->idle_handle = xTaskGetIdleTaskHandleForCPU(core_id);

    //trace->idle_handle = xTaskGetIdleTaskHandle();

    ets_printf("IDLE TASK %lx", trace->idle_handle);

}


void grid_trace_task_switched_in(void)
{
    struct grid_trace_model* trace = NULL;
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    uint8_t current_core = xTaskGetAffinity(current_task);

    if (current_core == 0){

        trace = &grid_trace_state_core0;
    }
    else if (current_core == 1){

        trace = &grid_trace_state_core1;
    }
    else{
        return;
    }

    //ets_printf("%lx\r\n", (unsigned long int) current_task);
    if (trace->last_active_handle != (unsigned long int) current_task){

        trace->last_active_handle = (unsigned long int) current_task;
        trace->switch_in_count++;

    }

}

void grid_trace_task_switched_out(void)
{
    struct grid_trace_model* trace = &grid_trace_state_core0;
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    //ets_printf("%lx\r\n", (unsigned long int) current_task);
    //trace->switch_out_count++;

    //

}


void grid_trace_report_task(void *arg)
{

    #ifndef gridUSE_TRACE
        vTaskSuspend(NULL);
        return;
    #endif

    grid_trace_init(&grid_trace_state_core0, 0);
    grid_trace_init(&grid_trace_state_core1, 1);

    char stats[3000] = {0};


    while(1){
        ets_printf("Switch: %d %d\r\n", grid_trace_state_core0.switch_in_count, grid_trace_state_core1.switch_in_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //Wait to be deleted
    vTaskSuspend(NULL);
}


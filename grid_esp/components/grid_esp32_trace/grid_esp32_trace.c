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
#include <string.h>
#include <stdint.h>

DRAM_ATTR struct grid_trace_model grid_trace_state_core0;
DRAM_ATTR struct grid_trace_model grid_trace_state_core1;

void grid_trace_init(struct grid_trace_model* trace, uint8_t core_id){

    trace->core_id = core_id;

    trace->ignored_task = NULL;

    trace->switch_in_count = 0;
    trace->switch_out_count = 0;

    trace->last_active_handle = 0;
    trace->idle_handle = xTaskGetIdleTaskHandleForCPU(core_id);

    memset(trace->trace_buffer, 0, sizeof(trace->trace_buffer));
    trace->trace_buffer_write_ptr = 0;
    trace->trace_buffer_read_ptr = 0;


    //trace->idle_handle = xTaskGetIdleTaskHandle();

    ets_printf("IDLE TASK %lx\r\n", trace->idle_handle);

}


    
void grid_trace_ignore_task(struct grid_trace_model* trace, void* task_handle){

    ets_printf("IGNORING %lx\r\n", (long unsigned int) task_handle);

    trace->ignored_task = task_handle;

}

void grid_trace_task_switched_in(void)
{
    struct grid_trace_model* trace = NULL;
    const uint32_t current_task = (uint32_t) xTaskGetCurrentTaskHandle();
    const uint8_t current_core = xTaskGetAffinity(current_task);

    if (current_core == 0){
        trace = &grid_trace_state_core0;
    }
    else if (current_core == 1){
        trace = &grid_trace_state_core1;
    }
    else{
        return;
    }



    if (trace->ignored_task == current_task || current_task==NULL){
        return;
    }

    if (trace->last_active_handle == current_task){
        return;
    }



    trace->last_active_handle = current_task;
    trace->switch_in_count++;

    //ets_printf("%lx\r\n", (unsigned long int) current_task);

    trace->trace_buffer[trace->trace_buffer_write_ptr].event_type =  (uint32_t) (current_task == trace->idle_handle); // switched in
    trace->trace_buffer[trace->trace_buffer_write_ptr].event_context = (uint32_t) current_task; // task handle
    trace->trace_buffer[trace->trace_buffer_write_ptr].timestamp = (uint32_t) esp_timer_get_time(); // timestamp
    trace->trace_buffer[trace->trace_buffer_write_ptr].event_value = 0; // no value

    trace->trace_buffer_write_ptr = (trace->trace_buffer_write_ptr+1)%1000;

 

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
    
    grid_trace_ignore_task(&grid_trace_state_core0, xTaskGetHandle( "esp_timer" ));

    char stats[3000] = {0};


    while(1){

        for (uint32_t i=0; i<500; i++){

            struct grid_trace_model* trace = &grid_trace_state_core0;

            if (trace->trace_buffer_read_ptr == trace->trace_buffer_write_ptr){
                break;
            }

            uint32_t index = trace->trace_buffer_read_ptr;
            uint32_t timestamp = trace->trace_buffer[index].timestamp/1000;
            uint32_t context = trace->trace_buffer[index].event_context;
            uint32_t event_type = trace->trace_buffer[index].event_type;

            ets_printf("%d %ldms 0x%lx %d\r\n", index, timestamp, context, event_type);
            trace->trace_buffer_read_ptr = (index+1)%1000;
        }


        ets_printf("Switch: %d %d\r\n", grid_trace_state_core0.switch_in_count, grid_trace_state_core1.switch_in_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //Wait to be deleted
    vTaskSuspend(NULL);
}


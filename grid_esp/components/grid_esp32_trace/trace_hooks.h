/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once


#ifndef __ASSEMBLER__

#include "grid_esp32_trace.h"

#endif /* def __ASSEMBLER__ */


/*
Patch instructions

apt update && apt install nano
nano /opt/esp/idf/components/freertos/config/include/freertos/FreeRTOSConfig.h
#include "../../../../../../../../../project/grid_esp/components/grid_esp32_trace/trace_hooks.h"

OR

sed -i '10s|.*|#include "../../../../../../../../../project/grid_esp/components/grid_esp32_trace/trace_hooks.h"|' /opt/esp/idf/components/freertos/config/include/freertos/FreeRTOSConfig.h

*/

#ifdef __cplusplus
extern "C" {
#endif

/*

#define gridUSE_TRACE 1

#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1


#define traceTASK_SWITCHED_IN()                                                    	\
    grid_trace_task_switched_in();

#define traceTASK_SWITCHED_OUT()                                                    	\
    grid_trace_task_switched_out();


*/


#ifdef __cplusplus
}
#endif

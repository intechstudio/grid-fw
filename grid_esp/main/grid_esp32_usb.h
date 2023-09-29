/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>


#include "esp_check.h"

#include "rom/ets_sys.h" // For ets_printf

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"


#include "tinyusb.h"
#include "driver/gpio.h"


#include "../../grid_common/include/grid_port.h"
#include "../../grid_common/include/grid_usb.h"
#include "../../grid_common/include/grid_sys.h"




#ifdef __cplusplus
extern "C" {
#endif





//------------- CLASS -------------//
#define CFG_TUD_CDC             1
#define CFG_TUD_HID             1
#define CFG_TUD_MIDI            1
#define CFG_TUD_MSC             0
#define CFG_TUD_VENDOR          0


/** Helper defines **/

void grid_esp32_usb_task(void *arg);
void grid_esp32_usb_init(void);

extern void grid_platform_sync1_pulse_send();


int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

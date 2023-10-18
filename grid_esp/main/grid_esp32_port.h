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
#include "task.h"
#include "esp_log.h"

#include "esp_freertos_hooks.h"

#include "tinyusb.h"
#include "driver/gpio.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"


#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_sys.h"


#include "driver/spi_slave.h"


uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length);

void grid_platform_sync1_pulse_send();

#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_port_task(void *arg);


#ifdef __cplusplus
}
#endif

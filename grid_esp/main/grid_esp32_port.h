/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>


#include "esp_check.h"


#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"


#include "tinyusb.h"
#include "driver/gpio.h"


#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_sys.h"


#include "driver/spi_slave.h"


#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_port_task(void *arg);


#ifdef __cplusplus
}
#endif

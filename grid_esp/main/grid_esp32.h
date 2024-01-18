/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "driver/rmt_encoder.h"
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_efuse.h"
#include "grid_esp32_nvm.h"
#include "grid_esp32_pins.h"
#include "rom/ets_sys.h" // For ets_printf

#include "bootloader_random.h"
#include "esp_random.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "hal/cpu_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct tskTaskControlBlock;

void grid_esp32_housekeeping_task(void *arg);

/**
 * @brief Read HWCFG register value on ESP32 based module
 *
 * @param[in] void void
 * @return
 *      - 32-bit unsigned value of HWCFG register
 */
uint32_t grid_platform_get_hwcfg();

/**
 * @brief Read CPUID register (mac address) value on ESP32 based module
 *
 * @param[in] void void
 * @return
 *      - 64-bit unsigned value of CPUID register
 */

void grid_platform_printf(char const *fmt, ...);

uint32_t grid_platform_get_id(uint32_t *return_array);

uint32_t grid_platform_get_id(uint32_t *return_array);
uint32_t grid_platform_get_hwcfg();
uint8_t grid_platform_get_random_8();
void grid_platform_delay_ms(uint32_t delay_milliseconds);
uint8_t grid_platform_get_reset_cause();

uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);

void grid_platform_system_reset();
void grid_platform_nvm_defrag();

uint8_t grid_platform_get_adc_bit_depth();

uint64_t grid_platform_rtc_get_micros(void);

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

uint32_t grid_platform_get_cycles();

uint32_t grid_platform_get_cycles_per_us();

void *grid_platform_allocate_volatile(size_t size);

#ifdef __cplusplus
}
#endif

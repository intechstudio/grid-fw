/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#include "driver/gpio.h"
#include "grid_esp32_pins.h"
#include "grid_esp32_nvm.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf
#include "esp_efuse.h"


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

uint32_t grid_platform_get_id(uint32_t* return_array);

uint32_t grid_platform_get_id(uint32_t* return_array);
uint32_t grid_platform_get_hwcfg();
uint8_t grid_platform_get_random_8();
void grid_platform_delay_ms(uint32_t delay_milliseconds);
uint8_t grid_platform_get_reset_cause();

uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);


void grid_platform_close_actionstring_file(void* file_pointer);

void* grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type);
uint16_t grid_platform_get_actionstring_file_size(void* file_pointer);
uint8_t grid_platform_get_actionstring_file_has_size(void* file_pointer);

uint32_t grid_platform_read_actionstring_file_contents(void* file_pointer, char* targetstring);
void grid_platform_delete_actionstring_file(void* file_pointer);
void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length);

uint8_t grid_platform_get_nvm_state();


void	grid_platform_clear_actionstring_files_from_page(uint8_t page);
void grid_platform_delete_actionstring_files_all();


uint8_t grid_platform_erase_nvm_next();



uint32_t grid_plaform_get_nvm_nextwriteoffset();

void grid_platform_system_reset();
void grid_platform_nvm_defrag();

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);

int32_t grid_platform_usb_midi_write_status(void);

uint8_t grid_platform_get_adc_bit_depth();



uint64_t grid_platform_rtc_get_micros(void);

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

uint32_t grid_platform_get_cycles();

uint32_t grid_platform_get_cycles_per_us();

void* grid_platform_allocate_volatile(size_t size);


int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);


int32_t grid_platform_usb_keyboard_keys_state_change(void* keys_desc, uint8_t keys_count);

#ifdef __cplusplus
}
#endif

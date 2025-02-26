/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "grid_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_nvm_model {};

extern struct grid_esp32_nvm_model grid_esp32_nvm_state;

extern uint32_t grid_platform_get_cycles();
extern uint32_t grid_platform_get_cycles_per_us();

void grid_esp32_nvm_task(void* arg);

void grid_esp32_nvm_init(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model* nvm, char* path);
void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model* nvm, uint8_t page, uint8_t element, uint8_t event, char* actionstring);

int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, union grid_ui_file_handle* file_handle);

void grid_esp32_nvm_erase(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model* nvm, uint8_t page);
void grid_esp32_nvm_clear_conf(const char* path);

void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle);

uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle);

uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring);

void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length);

int grid_platform_find_file(char* path, union grid_ui_file_handle* file_handle);
uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle);
int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size);
int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size);
int grid_platform_delete_file(union grid_ui_file_handle* file_handle);

uint8_t grid_platform_get_nvm_state();

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page);
void grid_platform_delete_actionstring_files_all();

uint8_t grid_platform_erase_nvm_next();

uint32_t grid_plaform_get_nvm_nextwriteoffset();

#ifdef __cplusplus
}
#endif

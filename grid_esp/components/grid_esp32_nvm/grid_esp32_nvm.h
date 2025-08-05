/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "grid_ui.h"

#include "grid_esp32_littlefs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_nvm_model {
  struct esp_littlefs_t efs;
};

extern struct grid_esp32_nvm_model grid_esp32_nvm_state;

extern uint32_t grid_platform_get_cycles();
extern uint32_t grid_platform_get_cycles_per_us();

void grid_esp32_nvm_task(void* arg);

void grid_esp32_nvm_mount(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model* nvm, char* path);
void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model* nvm, uint8_t page, uint8_t element, uint8_t event, char* actionstring);

void grid_esp32_nvm_erase(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model* nvm, uint8_t page);
void grid_esp32_nvm_clear_conf(const char* path);

// TODO check if all these definitions correspond to used functions

#ifdef __cplusplus
}
#endif

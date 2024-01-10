/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_nvm_model {};

extern struct grid_esp32_nvm_model grid_esp32_nvm_state;

extern uint32_t grid_platform_get_cycles();
extern uint32_t grid_platform_get_cycles_per_us();

void grid_esp32_nvm_task(void *arg);

void grid_esp32_nvm_init(struct grid_esp32_nvm_model *nvm);
void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model *nvm, char *path);
void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model *nvm, uint8_t page,
                                uint8_t element, uint8_t event,
                                char *actionstring);
void grid_esp32_nvm_read_config(struct grid_esp32_nvm_model *nvm, void *fp,
                                char *actionstring);
void *grid_esp32_nvm_find_file(struct grid_esp32_nvm_model *nvm, uint8_t page,
                               uint8_t element, uint8_t event);

uint16_t grid_esp32_nvm_get_file_size(struct grid_esp32_nvm_model *nvm,
                                      void *fp);
void grid_esp32_nvm_erase(struct grid_esp32_nvm_model *nvm);
void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model *nvm, uint8_t page);
uint8_t
grid_esp32_nvm_clear_next_file_from_page(struct grid_esp32_nvm_model *nvm,
                                         uint8_t page);

#ifdef __cplusplus
}
#endif

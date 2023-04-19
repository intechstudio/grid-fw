/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "rom/ets_sys.h" // For ets_printf


#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "spi_flash_mmap.h"

#include <string.h>

#include "esp_flash.h"


#include "esp_log.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf

#include "../managed_components/joltwallet__littlefs/include/esp_littlefs.h"


#include <sys/types.h>
#include <dirent.h>

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "../../grid_common/grid_ui.h"


#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_nvm_model{



};

extern struct grid_esp32_nvm_model* grid_esp32_nvm_state;


void grid_esp32_nvm_task(void *arg);

void grid_esp32_nvm_init(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model* nvm, uint8_t page, uint8_t element, uint8_t event, char* actionstring);
void grid_esp32_nvm_read_config(struct grid_esp32_nvm_model* nvm, void* fp, char* actionstring);
void* grid_esp32_nvm_find_file(struct grid_esp32_nvm_model* nvm, uint8_t page, uint8_t element, uint8_t event);

uint16_t grid_esp32_nvm_get_file_size(struct grid_esp32_nvm_model* nvm,  void* fp);
void grid_esp32_nvm_erase(struct grid_esp32_nvm_model* nvm);
void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model* nvm, uint8_t page);

#ifdef __cplusplus
}
#endif

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_nvm.h"

#include "grid_platform.h"
#include "grid_protocol.h"

#include "esp_system.h"
#include <sys/stat.h>
#include <sys/unistd.h>

#include <string.h>

#include "esp_flash.h"

#include "esp_check.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "grid_esp32_littlefs.h"
#include "grid_littlefs.h"

#include <sys/types.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include "driver/gpio.h"
#include "grid_ui.h"

static const char* TAG = "grid_esp32_nvm";

struct grid_esp32_nvm_model grid_esp32_nvm_state;

void grid_esp32_nvm_mount(struct grid_esp32_nvm_model* nvm, bool force_format) {

  // Initialize and mount littlefs
  esp_err_t ret = grid_esp32_littlefs_mount(&nvm->efs, force_format);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to initialize littlefs: %s", esp_err_to_name(ret));
    return;
  }

  // Set pointer to littlefs
  grid_platform_set_lfs(nvm->efs.lfs);

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->efs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->efs.lfs, &nvm->efs.cfg);
  ESP_LOGI(TAG, "littlefs size: total: %d, used: %d", total, used);

  // List the filesystem root
  grid_platform_list_directory("");
}

void grid_esp32_nvm_unmount(struct grid_esp32_nvm_model* nvm) {

  // Unmount littlefs
  esp_err_t ret = grid_esp32_littlefs_unmount(&nvm->efs);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to deinitialize littlefs: %s", esp_err_to_name(ret));
    return;
  }
}

void grid_platform_nvm_erase() {

  grid_platform_delete_actionstring_files_all();

  struct grid_file_t handle = {0};
  if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &handle) == 0) {
    grid_platform_delete_file(&handle);
  }
}

void grid_platform_nvm_format_and_mount() {

  grid_esp32_nvm_unmount(&grid_esp32_nvm_state);
  grid_esp32_nvm_mount(&grid_esp32_nvm_state, true);
}

const char* grid_platform_get_base_path() { return grid_esp32_nvm_state.efs.base_path; }

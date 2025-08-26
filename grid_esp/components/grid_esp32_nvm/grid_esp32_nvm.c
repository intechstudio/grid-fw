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

void grid_esp32_nvm_mount(struct grid_esp32_nvm_model* nvm) {

  // Initialize and mount littlefs
  esp_err_t ret = grid_esp32_littlefs_mount(&nvm->efs);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to initialize littlefs: %s", esp_err_to_name(ret));
    return;
  }

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->efs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->efs.lfs, &nvm->efs.cfg);
  ESP_LOGI(TAG, "littlefs size: total: %d, used: %d", total, used);

  // List the filesystem root
  grid_platform_list_directory("");
}

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle) {

  int ret = grid_littlefs_find_next_on_page(grid_esp32_nvm_state.efs.lfs, page, last_element, last_event);

  if (ret == 0) {

    sprintf(file_handle->fname, "%02x/%02x/%02x.cfg", page, *last_element, *last_event);
  }

  return ret;
}

int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event, union grid_ui_file_handle* file_handle) {

  char path[50] = {0};
  sprintf(path, "%02x/%02x/%02x.cfg", page, element, event);

  int status = grid_platform_find_file(path, file_handle);

  // Clear file handle if not found
  if (status) {
    memset(file_handle, 0, sizeof(union grid_ui_file_handle));
  }

  return status;
}

void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle) { grid_platform_delete_file(file_handle); }

uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle) { return grid_platform_get_file_size(file_handle); }

uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring, uint16_t size) {

  return grid_platform_read_file(file_handle, (uint8_t*)targetstring, size);
}

void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event, char* buffer, uint16_t length) {

  char path[50] = {0};

  sprintf(&path[strlen(path)], "%02x", page);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x", element);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x.cfg", event);
  int ret = grid_platform_write_file(path, (uint8_t*)buffer, length + 1);
}

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page) {

  char path[50] = {0};

  // upkeep: loop bound
  for (uint8_t i = 0; i < 18 + 1; ++i) {

    // Remove element directory
    sprintf(path, "%02x/%02x", page, i);
    grid_platform_remove_dir(path);
  }

  // Remove page directory
  sprintf(path, "%02x", page);
  grid_platform_remove_dir(path);
}

void grid_platform_delete_actionstring_files_all() {

  // upkeep: loop bound
  for (uint8_t i = 0; i < 4; ++i) {

    grid_platform_clear_all_actionstring_files_from_page(i);
  }
}

void grid_platform_nvm_erase() {

  grid_platform_delete_actionstring_files_all();

  union grid_ui_file_handle file_handle = {0};
  if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &file_handle) == 0) {
    grid_platform_delete_file(&file_handle);
  }
}

const char* grid_platform_get_base_path() { return grid_esp32_nvm_state.efs.base_path; }

int grid_platform_make_directory(const char* path) { return grid_littlefs_mkdir(grid_esp32_nvm_state.efs.lfs, path); }

int grid_platform_list_directory(const char* path) { return grid_littlefs_lsdir(grid_esp32_nvm_state.efs.lfs, path); }

int grid_platform_find_file(const char* path, union grid_ui_file_handle* file_handle) {

  int status = grid_littlefs_file_find(grid_esp32_nvm_state.efs.lfs, path);

  if (status == 0) {
    strcpy(file_handle->fname, path);
  }

  return status;
}

uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle) { return grid_littlefs_file_size(grid_esp32_nvm_state.efs.lfs, file_handle->fname); }

int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_read(grid_esp32_nvm_state.efs.lfs, file_handle->fname, buffer, size); }

int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_write(grid_esp32_nvm_state.efs.lfs, path, buffer, size); }

int grid_platform_delete_file(union grid_ui_file_handle* file_handle) { return grid_littlefs_remove(grid_esp32_nvm_state.efs.lfs, file_handle->fname); }

int grid_platform_remove_path(const char* path) { return grid_littlefs_remove(grid_esp32_nvm_state.efs.lfs, path); }

int grid_platform_remove_dir(const char* path) { return grid_littlefs_rmdir(grid_esp32_nvm_state.efs.lfs, path); }

uint8_t grid_platform_get_nvm_state() { return 1; }

uint8_t grid_platform_erase_nvm_next() { return 1; }

uint32_t grid_plaform_get_nvm_nextwriteoffset() { return 0; }

void grid_platform_nvm_defrag() {}

void grid_esp32_nvm_task(void* arg) {

  while (1) {

    if (false == grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {

      vTaskDelay(pdMS_TO_TICKS(15));
      continue;
    }

    uint64_t time_max_duration = 150 * 1000; // in microseconds
    uint64_t time_start = grid_platform_rtc_get_micros();

    do {

      switch (grid_ui_get_bulk_status(&grid_ui_state)) {
      case GRID_UI_BULK_READ_PROGRESS:
        grid_ui_bulk_pageread_next(&grid_ui_state);
        break;
      case GRID_UI_BULK_STORE_PROGRESS:
        grid_ui_bulk_pagestore_next(&grid_ui_state);
        break;
      case GRID_UI_BULK_CLEAR_PROGRESS:
        grid_ui_bulk_pageclear_next(&grid_ui_state);
        break;
      case GRID_UI_BULK_CONFREAD_PROGRESS:
        grid_ui_bulk_confread_next(&grid_ui_state);
        break;
      case GRID_UI_BULK_CONFSTORE_PROGRESS:
        grid_ui_bulk_confstore_next(&grid_ui_state);
        break;
      case GRID_UI_BULK_ERASE_PROGRESS:
        grid_ui_bulk_nvmerase_next(&grid_ui_state);
        break;
      default:
        break;
      }

    } while (grid_platform_rtc_get_elapsed_time(time_start) < time_max_duration && grid_ui_bulk_anything_is_in_progress(&grid_ui_state));

    vTaskDelay(pdMS_TO_TICKS(15));
  }

  ESP_LOGI(TAG, "Deinit NVM");

  // Wait to be deleted
  vTaskSuspend(NULL);
}

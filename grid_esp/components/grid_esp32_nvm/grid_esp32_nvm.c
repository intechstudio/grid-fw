/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_nvm.h"

#include "grid_platform.h"
#include "grid_protocol.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_system.h"
#include <sys/stat.h>
#include <sys/unistd.h>

#include <string.h>

#include "esp_flash.h"

#include "esp_check.h"
#include "esp_log.h"
#include "rom/ets_sys.h" // For ets_printf

#include "esp_littlefs.h"

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

void grid_esp32_nvm_mount() {

  esp_vfs_littlefs_conf_t conf = {
      .base_path = "/littlefs",
      .partition_label = "ffat",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  // Use an all-in-one convenience function to initialize and mount littlefs
  esp_err_t ret = esp_vfs_littlefs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find LittleFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}

void grid_esp32_nvm_init(struct grid_esp32_nvm_model* nvm) {

  grid_esp32_nvm_mount();

  grid_esp32_nvm_list_files(NULL, "/littlefs");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00/00");
}

void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model* nvm, char* path) {

  ESP_LOGI(TAG, "Print Directory: %s", path);

  DIR* d;
  struct dirent* dir;
  d = opendir(path);

  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }

  return;
}

void grid_esp32_nvm_erase(struct grid_esp32_nvm_model* nvm) {

  grid_esp32_nvm_clear_page(nvm, 0);
  grid_esp32_nvm_clear_page(nvm, 1);
  grid_esp32_nvm_clear_page(nvm, 2);
  grid_esp32_nvm_clear_page(nvm, 3);

  grid_esp32_nvm_clear_conf(GRID_UI_CONFIG_PATH);
}

void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model* nvm, uint8_t page) {

  // upkeep: loop bound
  for (uint8_t i = 0; i < 18 + 1; ++i) {

    // upkeep: loop bound
    for (uint8_t j = 0; j < 10; ++j) {

      union grid_ui_file_handle file_handle = {0};
      int status = grid_platform_find_actionstring_file(page, i, j, &file_handle);

      if (status == 0) {
        grid_platform_delete_file(&file_handle);
      }
    }
  }
}

void grid_esp32_nvm_clear_conf(const char* path) {

  int status;

  union grid_ui_file_handle file_handle = {0};

  status = grid_platform_find_file(path, &file_handle);
  if (status == 0) {
    grid_platform_delete_file(&file_handle);
  }
}

static int scandir2(const char* dirp, struct dirent*** namelist, int (*filter)(struct dirent*), int (*compar)(const void*, const void*)) {

  struct dirent** names = NULL;
  size_t items = 0;
  DIR* dir = NULL;

  // Allocate entry names, with an initial guess for size
  size_t array_size = 20;
  names = malloc(array_size * sizeof(struct dirent*));
  if (!names) {
    goto scandir_cleanup;
  }

  dir = opendir(dirp);
  if (!dir) {
    goto scandir_cleanup;
  }

  struct dirent* entry = NULL;

  while ((entry = readdir(dir)) != NULL) {

    // If there is a filter function, skip non-matching entry
    if (filter && !filter(entry)) {
      continue;
    }

    // Allocate entry to be returned
    struct dirent* entry_copy = malloc(sizeof(struct dirent));
    if (!entry_copy) {
      goto scandir_cleanup;
    }

    *entry_copy = *entry;
    names[items++] = entry_copy;

    // Reallocate the array if it becomes full
    if (items >= array_size) {

      array_size += 10;

      struct dirent** new_names = realloc(names, array_size * sizeof(struct dirent*));
      if (!new_names) {
        goto scandir_cleanup;
      }

      names = new_names;
    }
  }

  // Sort entries if a comparison function was provided
  if (items && compar) {
    qsort(names, items, sizeof(struct dirent*), compar);
  }

  closedir(dir);

  *namelist = names;

  return items;

scandir_cleanup:

  if (names) {

    while (items > 0) {
      free(names[--items]);
    }
    free(names);
  }

  if (dir) {
    closedir(dir);
  }

  return -1;
}

static int my_alphasort(const void* d1, const void* d2) { return (strcmp((*(struct dirent**)d1)->d_name, (*(struct dirent**)d2)->d_name)); }

static int find_next_file(char* path, int* last_file_number) {

  // Must init to null to avoid freeing random memory
  struct dirent** entries = NULL;

  // Scan directory entries with an alphabetical sort
  int entries_length = scandir2(path, &entries, NULL, my_alphasort);

  uint8_t i = 0;
  while (i < entries_length) {

    // Attempt to parse the entry name as two hexadecimal digits
    int element;
    int matched = sscanf(entries[i]->d_name, "%02x", &element);

    // Upon finding a larger index than the last file number, stop
    if (matched == 1 && element > *last_file_number) {
      break;
    }

    ++i;
  }

  // Deallocate entries
  if (entries) {

    for (uint8_t i = 0; i < entries_length; ++i) {
      free(entries[i]);
    }
    free(entries);
  }

  int found = i < entries_length;

  // Set last_file_number according to whether the next file was found
  *last_file_number = found ? i : -1;

  return found ? 0 : 1;
}

int grid_esp32_nvm_find_next_file_from_page(struct grid_esp32_nvm_model* nvm, uint8_t page, int* last_element, int* last_event) {

  char path[50] = {0};

  while (true) {

    if (*last_event == -1) {

      // Try to find next valid element
      sprintf(path, "/littlefs/%02x", page);
      find_next_file(path, last_element);

      // If no element is found, the traversal is over
      if (*last_element == -1) {
        return 1;
      }
    }

    // Try to find next valid event
    sprintf(path, "/littlefs/%02x/%02x", page, *last_element);
    find_next_file(path, last_event);

    // If a valid event is found, return success
    if (*last_event != -1) {
      return 0;
    }
  }
}

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle) {

  if (grid_esp32_nvm_find_next_file_from_page(&grid_esp32_nvm_state, page, last_element, last_event)) {
    return 1;
  }

  sprintf(file_handle->fname, "/littlefs/%02x/%02x/%02x.cfg", page, *last_element, *last_event);

  return 0;
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

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page) { grid_esp32_nvm_clear_page(&grid_esp32_nvm_state, page); };

void grid_platform_delete_actionstring_files_all() { grid_esp32_nvm_erase(&grid_esp32_nvm_state); }

int grid_esp32_nvm_build_path(const char* path, uint16_t out_size, char* out) {

  assert(path != out);

  char prefix[] = "/littlefs/";

  if (strlen(prefix) + strlen(path) + 1 > out_size) {
    return 1;
  }

  sprintf(out, "%s%s", prefix, path);

  return 0;
}

int grid_platform_make_directory(const char* pathname) {

  char path[50] = {0};
  grid_esp32_nvm_build_path(pathname, 50, path);

  if (mkdir(path, 0777) == -1) {
    return 1;
  }

  return 0;
}

int grid_platform_find_file(const char* path, union grid_ui_file_handle* file_handle) {

  grid_esp32_nvm_build_path(path, 50, file_handle->fname);

  FILE* fp = fopen(file_handle->fname, "r");

  if (!fp) {
    // printf("FILE FIND ERROR\r\n");
    memset(file_handle, 0, sizeof(union grid_ui_file_handle));
    return 1;
  }

  fclose(fp);

  return 0;
}

uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle) {

  FILE* fp = fopen(file_handle->fname, "r");

  if (!fp) {
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  uint32_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  fclose(fp);

  return size;
}

int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size) {

  FILE* fp = fopen(file_handle->fname, "r");

  if (!fp) {
    return 1;
  }

  int ret = fread(buffer, size, 1, fp);

  fclose(fp);

  if (ret != 1) {
    printf("FILE READ ERROR\r\n");
    return 1;
  }

  return 0;
}

int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size) {

  char fname[50] = {0};

  grid_esp32_nvm_build_path(path, 50, fname);

  FILE* fp = fopen(fname, "w");

  if (!fp) {
    printf("FILE OPEN ERROR\r\n");
    return 1;
  }

  int ret = fwrite(buffer, size, 1, fp);

  fclose(fp);

  if (ret != 1) {
    printf("FILE WRITE ERROR\r\n");
    return 1;
  }

  return 0;
}

int grid_platform_delete_file(union grid_ui_file_handle* file_handle) { return unlink(file_handle->fname); }

uint8_t grid_platform_get_nvm_state() { return 1; }

uint8_t grid_platform_erase_nvm_next() {

  ets_printf("ERASE WAS ALREADY DONE ON INIT!!!\r\n");

  return 1;
}

uint32_t grid_plaform_get_nvm_nextwriteoffset() {

  ets_printf("grid_plaform_get_nvm_nextwriteoffset NOT IMPLEMENTED!!!\r\n");

  return 0;
}

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

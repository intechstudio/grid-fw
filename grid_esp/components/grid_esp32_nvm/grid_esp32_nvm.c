/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_nvm.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_chip_info.h"
#include "esp_system.h"
#include <sys/stat.h>
#include <sys/unistd.h>

#include <string.h>

#include "esp_flash.h"

#include "esp_check.h"
#include "esp_log.h"
#include "rom/ets_sys.h" // For ets_printf

#include "esp_littlefs.h"

#include <dirent.h>
#include <sys/types.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#include "grid_ui.h"

static const char *TAG = "grid_esp32_nvm";

struct grid_esp32_nvm_model grid_esp32_nvm_state;

void grid_esp32_nvm_print_chip_info() {

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU cores, WiFi%s%s, ", CONFIG_IDF_TARGET,
         chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  uint32_t size_flash_chip = 0;
  esp_flash_get_size(NULL, &size_flash_chip);
  printf("%uMB %s flash\n", (unsigned int)size_flash_chip >> 20,
         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                       : "external");

  printf("Free heap: %u\n", (unsigned int)esp_get_free_heap_size());
}

void grid_esp32_nvm_mount() {

  esp_vfs_littlefs_conf_t conf = {
      .base_path = "/littlefs",
      .partition_label = "ffat",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  // Use settings defined above to initialize and mount LittleFS filesystem.
  // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
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
    ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)",
             esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}

void grid_esp32_nvm_read_write_test() {

  grid_esp32_nvm_list_files(NULL, "/littlefs");

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(TAG, "Opening file");
  FILE *f = fopen("/littlefs/hello.txt", "w");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  fprintf(f, "LittleFS Rocks!\n");
  fclose(f);
  ESP_LOGI(TAG, "File written");

  grid_esp32_nvm_list_files(NULL, "/littlefs");

  // Check if destination file exists before renaming
  struct stat st;
  if (stat("/littlefs/foo.txt", &st) == 0) {
    // Delete it if it exists
    unlink("/littlefs/foo.txt");
  }

  // Rename original file
  ESP_LOGI(TAG, "Renaming file");
  if (rename("/littlefs/hello.txt", "/littlefs/foo.txt") != 0) {
    ESP_LOGE(TAG, "Rename failed");
    return;
  }

  grid_esp32_nvm_list_files(NULL, "/littlefs");

  // Open renamed file for reading
  ESP_LOGI(TAG, "Reading file");
  f = fopen("/littlefs/foo.txt", "r");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
  }
  char line[64];
  fgets(line, sizeof(line), f);
  fclose(f);
  // strip newline
  char *pos = strchr(line, '\n');
  if (pos) {
    *pos = '\0';
  }
  ESP_LOGI(TAG, "Read from file: '%s'", line);

  grid_esp32_nvm_list_files(NULL, "/littlefs");
}

void grid_esp32_nvm_init(struct grid_esp32_nvm_model *nvm) {

  // grid_esp32_nvm_print_chip_info();

  grid_esp32_nvm_mount();

  // esp_littlefs_format("ffat");
  grid_esp32_nvm_list_files(NULL, "/littlefs");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00/00");
  // grid_esp32_nvm_read_write_test();
}

void grid_esp32_nvm_list_files(struct grid_esp32_nvm_model *nvm, char *path) {

  ESP_LOGI(TAG, "Print Directory: %s", path);

  DIR *d;
  struct dirent *dir;
  d = opendir(path);

  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }

  return;
}

void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model *nvm, uint8_t page,
                                uint8_t element, uint8_t event,
                                char *actionstring) {

  char fname[30] = {0};

  sprintf(fname, "/littlefs/%02x", page);

  if (mkdir(fname, 0777) == -1) {
    // printf("Error creating directory.\n");
  }

  sprintf(fname, "/littlefs/%02x/%02x", page, element);

  if (mkdir(fname, 0777) == -1) {
    // printf("Error creating directory.\n");
  }

  sprintf(fname, "/littlefs/%02x/%02x/%02x.cfg", page, element, event);

  ESP_LOGD(TAG, "%s : %s", fname, actionstring);

  FILE *fp;

  fp = fopen(fname, "w");

  if (fp) {

    printf("FILE OK\r\n");
    fprintf(fp, "%s", actionstring);
    fclose(fp);
  } else {
    printf("FILE ERROR\r\n");
  }
}

void grid_esp32_nvm_read_config(struct grid_esp32_nvm_model *nvm, void *fp,
                                char *actionstring) {

  ESP_LOGD(TAG, "TRY READ FILE %lx to %lx\r\n", (long unsigned int)fp,
           (long unsigned int)actionstring);

  if (fp) {

    fgets(actionstring, GRID_PARAMETER_ACTIONSTRING_maxlength, fp);
    ESP_LOGD(TAG, "FREAD ACTION %s\r\n", actionstring);

  } else {

    ESP_LOGD(TAG, "FREAD NO FILE \r\n");
  }
}

void *grid_esp32_nvm_find_file(struct grid_esp32_nvm_model *nvm, uint8_t page,
                               uint8_t element, uint8_t event) {

  char fname[30] = {0};

  sprintf(fname, "/littlefs/%02x/%02x/%02x.cfg", page, element, event);

  FILE *fp;

  fp = fopen(fname, "r");

  return fp;
}

uint16_t grid_esp32_nvm_get_file_size(struct grid_esp32_nvm_model *nvm,
                                      void *fp) {

  if (fp) {

    fseek(fp, 0, SEEK_END);    // seek to end of file
    uint32_t size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET);    // seek back to beginning of file

    return size;
  } else {
    ets_printf("INVALID FP\r\n");
    return 0;
  }
}

void grid_esp32_nvm_erase(struct grid_esp32_nvm_model *nvm) {

  grid_esp32_nvm_clear_page(nvm, 0);
  grid_esp32_nvm_clear_page(nvm, 1);
  grid_esp32_nvm_clear_page(nvm, 2);
  grid_esp32_nvm_clear_page(nvm, 3);
}

void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model *nvm, uint8_t page) {

  for (uint8_t i = 0; i < 17; i++) { // elements

    for (uint8_t j = 0; j < 10; j++) { // events

      FILE *fp = grid_esp32_nvm_find_file(nvm, page, i, j);

      if (fp != NULL) {

        fclose(fp);

        char fname[30] = {0};

        sprintf(fname, "/littlefs/%02x/%02x/%02x.cfg", page, i, j);

        unlink(fname);

        ets_printf("DELETE: %s\r\n", fname);
      }
    }
  }
}

uint8_t
grid_esp32_nvm_clear_next_file_from_page(struct grid_esp32_nvm_model *nvm,
                                         uint8_t page) {

  char path[15] = {0};
  sprintf(path, "/littlefs/%02x", page);

  DIR *d;
  struct dirent *dir;
  d = opendir(path);

  if (!d) {
    // Directory not found
    return 1;
  }

  // treverse filesystem with depth of two directories to quickly find first
  // file to be deleted!
  while ((dir = readdir(d)) != NULL) {
    printf("  %s\n", dir->d_name);
    char path2[300] = {0};
    sprintf(path2, "%s/%s", path, dir->d_name);
    DIR *d2;
    struct dirent *dir2;
    d2 = opendir(path2);

    if (d2) {
      while ((dir2 = readdir(d2)) != NULL) {
        printf("    %s\n", dir2->d_name);
        char fname[600] = {0};
        sprintf(fname, "%s/%s", path2, dir2->d_name);
        printf("Delete: %s\n", fname);
        unlink(fname);

        closedir(d2);
        closedir(d);
        return 0;
      }
      closedir(d2);
    }
  }
  closedir(d);

  return 1;
}

#include "grid_esp32_port.h"

void grid_esp32_nvm_task(void *arg) {

  SemaphoreHandle_t nvm_or_port = (SemaphoreHandle_t)arg;

  static uint32_t loopcounter = 0;

  while (1) {

    if (false == grid_ui_bluk_anything_is_in_progress(&grid_ui_state)) {

      vTaskDelay(pdMS_TO_TICKS(15));
      continue;
    }

    if (xSemaphoreTake(nvm_or_port, portMAX_DELAY) == pdTRUE) {

      uint64_t time_max_duration = 150 * 1000; // in microseconds
      uint64_t time_start = grid_platform_rtc_get_micros();
      uint32_t counter = 0;

      struct grid_port *ui_port = grid_transport_get_port_first_of_type(
          &grid_transport_state, GRID_PORT_TYPE_UI);

      do {

        // NVM BULK STORE
        if (grid_ui_bulk_pagestore_is_in_progress(&grid_ui_state)) {

          grid_ui_bulk_pagestore_next(&grid_ui_state);
        } else if (grid_ui_bulk_nvmerase_is_in_progress(&grid_ui_state)) {

          grid_ui_bulk_nvmerase_next(&grid_ui_state);
        } else if (grid_ui_bulk_pageclear_is_in_progress(&grid_ui_state)) {

          grid_ui_bulk_pageclear_next(&grid_ui_state);
        } else if (ui_port->rx_double_buffer_status == 0 &&
                   grid_ui_bulk_pageread_is_in_progress(&grid_ui_state)) {
          grid_ui_bulk_pageread_next(&grid_ui_state);

        } else {
          break;
        }

        // grid_esp32_port_periodic_ping_heartbeat_handler_cb(NULL);

        counter++;

      } while (grid_platform_rtc_get_elapsed_time(time_start) <
               time_max_duration);

      xSemaphoreGive(nvm_or_port);
    }

    vTaskDelay(pdMS_TO_TICKS(15));
  }

  ESP_LOGI(TAG, "Deinit NVM");

  // Wait to be deleted
  xSemaphoreGive(nvm_or_port);
  vTaskSuspend(NULL);
}

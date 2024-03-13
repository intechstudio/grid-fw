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

void grid_esp32_nvm_print_chip_info() {

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU cores, WiFi%s%s, ", CONFIG_IDF_TARGET, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  uint32_t size_flash_chip = 0;
  esp_flash_get_size(NULL, &size_flash_chip);
  printf("%uMB %s flash\n", (unsigned int)size_flash_chip >> 20, (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

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
    ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}

void grid_esp32_nvm_read_write_test() {

  grid_esp32_nvm_list_files(NULL, "/littlefs");

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(TAG, "Opening file");
  FILE* f = fopen("/littlefs/hello.txt", "w");
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
  char* pos = strchr(line, '\n');
  if (pos) {
    *pos = '\0';
  }
  ESP_LOGI(TAG, "Read from file: '%s'", line);

  grid_esp32_nvm_list_files(NULL, "/littlefs");
}

void grid_esp32_nvm_init(struct grid_esp32_nvm_model* nvm) {

  // grid_esp32_nvm_print_chip_info();

  grid_esp32_nvm_mount();

  // esp_littlefs_format("ffat");
  grid_esp32_nvm_list_files(NULL, "/littlefs");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00");
  grid_esp32_nvm_list_files(NULL, "/littlefs/00/00");
  // grid_esp32_nvm_read_write_test();
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

void grid_esp32_nvm_save_config(struct grid_esp32_nvm_model* nvm, uint8_t page, uint8_t element, uint8_t event, char* actionstring) {

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

  FILE* fp;

  fp = fopen(fname, "w");

  if (fp) {

    printf("FILE OK\r\n");
    fprintf(fp, "%s", actionstring);
    fclose(fp);
  } else {
    printf("FILE ERROR\r\n");
  }
}

int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, union grid_ui_file_handle* file_handle) {

  sprintf(file_handle->fname, "/littlefs/%02x/%02x/%02x.cfg", page, element, event_type);

  // check if file exists
  FILE* fp;
  fp = fopen(file_handle->fname, "r");

  if (fp != NULL) {
    fclose(fp);
    return 0;
  }

  // clear file_handle value
  memset(file_handle, 0, sizeof(union grid_ui_file_handle));
  return 1;
}

void grid_esp32_nvm_erase(struct grid_esp32_nvm_model* nvm) {

  grid_esp32_nvm_clear_page(nvm, 0);
  grid_esp32_nvm_clear_page(nvm, 1);
  grid_esp32_nvm_clear_page(nvm, 2);
  grid_esp32_nvm_clear_page(nvm, 3);
}

void grid_esp32_nvm_clear_page(struct grid_esp32_nvm_model* nvm, uint8_t page) {

  for (uint8_t i = 0; i < 17; i++) { // elements

    for (uint8_t j = 0; j < 10; j++) { // events

      union grid_ui_file_handle file_handle = {0};
      int status = grid_platform_find_actionstring_file(page, i, j, &file_handle);

      if (status == 0) {

        unlink(file_handle.fname);
        ets_printf("DELETE: %s\r\n", file_handle.fname);
      }
    }
  }
}

static int scandir2(const char* dirname, struct dirent*** namelist, int (*select)(struct dirent*), int (*dcomp)(const void*, const void*)) {

  struct dirent *d, *p, **names = NULL;
  size_t nitems = 0;
  struct stat stb;
  long arraysz;
  DIR* dirp;

  if ((dirp = opendir(dirname)) == NULL) {

    printf("OPENDIR FAILED\r\n");
    return (-1);
  }

  /*
   * estimate the array size by taking the size of the directory file
   * and dividing it by a multiple of the minimum size entry.
   */

  arraysz = (20);
  names = (struct dirent**)malloc(arraysz * sizeof(struct dirent*));
  if (names == NULL)
    goto fail;

  while ((d = readdir(dirp)) != NULL) {
    if (select != NULL && !(*select)(d))
      continue; /* just selected names */
    /*
     * Make a minimum size copy of the data
     */
    p = (struct dirent*)malloc(sizeof(struct dirent));
    if (p == NULL)
      goto fail;
    p->d_type = d->d_type;

    bcopy(d->d_name, p->d_name, 255 + 1);
    /*
     * Check to make sure the array has space left and
     * realloc the maximum size.
     */
    if (nitems >= arraysz) {
      const int inc = 10; /* increase by this much */
      struct dirent** names2;

      names2 = (struct dirent**)realloc((char*)names, (arraysz + inc) * sizeof(struct dirent*));
      if (names2 == NULL) {
        free(p);
        goto fail;
      }
      names = names2;
      arraysz += inc;
    }
    names[nitems++] = p;
  }
  closedir(dirp);
  if (nitems && dcomp != NULL)
    qsort(names, nitems, sizeof(struct dirent*), dcomp);
  *namelist = names;
  return (nitems);

fail:
  while (nitems > 0)
    free(names[--nitems]);
  free(names);
  closedir(dirp);
  return -1;
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
static int my_alphasort(const void* d1, const void* d2) { return (strcmp((*(struct dirent**)d1)->d_name, (*(struct dirent**)d2)->d_name)); }

static int find_next_file(char* path, char* file_name_fmt, int* last_file_number) {

  uint8_t file_found_already = false;

  struct dirent** entries = NULL; // must init with null to avoid freeing random memory;
  int entries_length;
  entries_length = scandir2(path, &entries, NULL, my_alphasort);

  for (uint8_t i = 0; i < entries_length; i++) {

    // printf("files: %s\r\n", entries[i]->d_name);

    int element;
    int element_match = sscanf(entries[i]->d_name, "%02x", &element);
    free(entries[i]);

    if (file_found_already) {
      continue;
    }

    if (element_match != 1) {
      continue;
    }

    if (element <= *last_file_number) {
      continue;
    }

    // MATCH
    file_found_already = true;
    *last_file_number = element;
  }

  if (entries != NULL) {
    free(entries);
  }

  if (file_found_already) {
    return 0;
  }

  *last_file_number = -1; // indicating that search is completed and no more files are available

  return 1;
}

int grid_esp32_nvm_find_next_file_from_page(struct grid_esp32_nvm_model* nvm, uint8_t page, int* last_element, int* last_event) {

  while (true) {

    if (*last_event == -1) {

      // try to find valid element
      char path[25] = {0};
      sprintf(path, "/littlefs/%02x", page);
      find_next_file(path, "%02x", last_element);

      if (*last_element == -1) {
        // no more element is found, traversal is over
        return 1;
      }
    }

    // try to find valid event
    char path[25] = {0};
    sprintf(path, "/littlefs/%02x/%02x", page, *last_element);
    find_next_file(path, "%02x.cfg", last_event);

    if (*last_event != -1) {
      // valid event is found, return success
      return 0;
    }
  }
}

void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle) {

  unlink(file_handle->fname);
  return;
}

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle) {

  if (0 == grid_esp32_nvm_find_next_file_from_page(&grid_esp32_nvm_state, page, last_element, last_event)) {

    sprintf(file_handle->fname, "/littlefs/%02x/%02x/%02x.cfg", page, *last_element, *last_event);

    return 0;
  }

  return 1;
}

uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle) {

  FILE* fp = fopen(file_handle->fname, "r");

  if (fp) {

    fseek(fp, 0, SEEK_END);    // seek to end of file
    uint32_t size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET);    // seek back to beginning of file

    fclose(fp);

    return size;
  }

  ets_printf("INVALID FP\r\n");
  return 0;
}

uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring) {

  // ets_printf("READ FILE \r\n");

  FILE* fp = fopen(file_handle->fname, "r");

  if (fp) {

    fgets(targetstring, GRID_PARAMETER_ACTIONSTRING_maxlength, fp);
    fclose(fp);
  } else {

    ESP_LOGD(TAG, "FREAD NO FILE \r\n");
  }

  return 0;
}

void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length) {
  grid_esp32_nvm_save_config(&grid_esp32_nvm_state, page, element, event_type, buffer);

  return;
}

uint8_t grid_platform_get_nvm_state() {
  return 1; // ready, always ready
}

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page) {

  grid_esp32_nvm_clear_page(&grid_esp32_nvm_state, page);
  return;
};

void grid_platform_delete_actionstring_files_all() {

  grid_esp32_nvm_erase(&grid_esp32_nvm_state);

  return;
}

uint8_t grid_platform_erase_nvm_next() {

  ets_printf("ERASE WAS ALREADY DONE ON INIT!!!\r\n");

  return 1; // done
}

uint32_t grid_plaform_get_nvm_nextwriteoffset() {

  ets_printf("grid_plaform_get_nvm_nextwriteoffset NOT IMPLEMENTED!!!\r\n");
  return 0; // done
}

void grid_esp32_nvm_task(void* arg) {

  SemaphoreHandle_t nvm_or_port = (SemaphoreHandle_t)arg;

  static uint32_t loopcounter = 0;

  // gpio_set_direction(47, GPIO_MODE_OUTPUT);

  while (1) {

    if (false == grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {

      vTaskDelay(pdMS_TO_TICKS(15));
      continue;
    }

    if (xSemaphoreTake(nvm_or_port, portMAX_DELAY) == pdTRUE) {

      // gpio_set_level(47, 1);

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
        case GRID_UI_BULK_ERASE_PROGRESS:
          grid_ui_bulk_nvmerase_next(&grid_ui_state);
          break;
        default:
          break;
        }

      } while (grid_platform_rtc_get_elapsed_time(time_start) < time_max_duration && grid_ui_bulk_anything_is_in_progress(&grid_ui_state));

      // gpio_set_level(47, 0);

      xSemaphoreGive(nvm_or_port);
    }

    vTaskDelay(pdMS_TO_TICKS(15));
  }

  ESP_LOGI(TAG, "Deinit NVM");

  // Wait to be deleted
  xSemaphoreGive(nvm_or_port);
  vTaskSuspend(NULL);
}

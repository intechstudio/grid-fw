/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_littlefs.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "esp32s3/rom/spi_flash.h"

#include "rom/ets_sys.h"

enum { GRID_ESP32_LITTLEFS_PAGE_SIZE = 256 };
enum { GRID_ESP32_LITTLEFS_BLOCK_SIZE = 4096 };

static const char* TAG = "grid_esp32_littlefs";

static esp_err_t grid_esp32_littlefs_init(struct esp_littlefs_t* efs, lfs_t* lfs, const esp_partition_t* partition, const char* base_path, bool read_only) {

  struct lfs_config lfs_cfg = (struct lfs_config){

      .context = efs,

      // Block device operations
      .read = littlefs_api_read,
      .prog = littlefs_api_prog,
      .erase = littlefs_api_erase,
      .sync = littlefs_api_sync,

      // Block device configuration
      .read_size = 128,
      .prog_size = 128,
      .block_size = GRID_ESP32_LITTLEFS_BLOCK_SIZE,
      .block_count = partition->size / GRID_ESP32_LITTLEFS_BLOCK_SIZE,
      .cache_size = 512,
      .lookahead_size = 128,
      .block_cycles = 512,
      // .disk_version = 0x00020001,
  };

  {
    efs->lfs = lfs;
    efs->cfg = lfs_cfg;
    efs->partition = partition;
    efs->read_only = read_only;
  }

  strcpy(efs->base_path, base_path);

  return ESP_OK;
}

static esp_err_t grid_esp32_littlefs_mkdir_base(struct esp_littlefs_t* efs) {

  assert(efs->base_path[0] != '\0');

  // Attempt to stat the directory at the base path
  struct lfs_info info;
  int lfs_err = lfs_stat(efs->lfs, efs->base_path, &info);
  if (lfs_err == LFS_ERR_OK) {
    ESP_LOGI(TAG, "directory at base path already exists");
    return ESP_OK;
  }

  // Attempt to make directory at the base path
  ESP_LOGI(TAG, "creating directory at base path...");
  lfs_err = lfs_mkdir(efs->lfs, efs->base_path);
  if (lfs_err != LFS_ERR_OK) {
    ESP_LOGW(TAG, "failed to make directory at base path");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t grid_esp32_littlefs_mount(struct esp_littlefs_t* efs) {

  // Allocate littlefs
  lfs_t* lfs = malloc(sizeof(lfs_t));
  if (!lfs) {
    ESP_LOGE(TAG, "failed to allocate littlefs");
    return ESP_ERR_NO_MEM;
  }

  size_t type = ESP_PARTITION_TYPE_DATA;
  size_t subtype = ESP_PARTITION_SUBTYPE_ANY;
  const char* label = "ffat";
  esp_partition_iterator_t iter = esp_partition_find(type, subtype, label);

  // There must be at least one ffat partition
  if (!iter) {
    free(lfs);
    ESP_LOGE(TAG, "no data type partition with ffat label was found");
    return ESP_ERR_NOT_FOUND;
  }

  // There must be no more ffat partitions
  if (esp_partition_next(iter)) {
    free(lfs);
    ESP_LOGE(TAG, "unexpected data type partition with ffat label was found");
    return ESP_FAIL;
  }

  const esp_partition_t* part = esp_partition_get(iter);
  ESP_LOGI(TAG, "partition address: %08x, size: %lu", (unsigned int)part->address, part->size);

  // Check that the littlefs page size is evenly divisible by the flash chip page size
  if (GRID_ESP32_LITTLEFS_PAGE_SIZE % g_rom_flashchip.page_size != 0) {
    free(lfs);
    ESP_LOGE(TAG, "littlefs page size not evenly divisble by flash chip page size");
    return ESP_ERR_INVALID_ARG;
  }

  grid_esp32_littlefs_init(efs, lfs, part, "/littlefs", false);

  // Mount littlefs
  int lfs_err = lfs_mount(efs->lfs, &efs->cfg);
  // int lfs_err = LFS_ERR_OK + 1; // for testing purposes

  // If mounting failed, attempt a format and another mount
  if (lfs_err != LFS_ERR_OK) {

    ESP_LOGW(TAG, "littlefs mount failed (%d): %s. formatting...", lfs_err, littlefs_errno(lfs_err));

    lfs_err = lfs_format(efs->lfs, &efs->cfg);
    if (lfs_err != LFS_ERR_OK) {
      free(lfs);
      ESP_LOGE(TAG, "littlefs format failed");
      return ESP_FAIL;
    }

    ESP_LOGW(TAG, "littlefs format successful. mounting...");

    lfs_err = lfs_mount(efs->lfs, &efs->cfg);
    if (lfs_err != LFS_ERR_OK) {
      free(lfs);
      ESP_LOGE(TAG, "littlefs mount failed (%d): %s. exiting...", lfs_err, littlefs_errno(lfs_err));
      return ESP_FAIL;
    }
  }

  // Create directory at base path if it does not already exist
  grid_esp32_littlefs_mkdir_base(efs);

  return ESP_OK;
}

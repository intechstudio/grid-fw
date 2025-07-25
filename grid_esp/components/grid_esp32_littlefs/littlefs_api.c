#include "littlefs_api.h"

#include "esp_log.h"
#include "esp_partition.h"
#include "lfs.h"

static const char* TAG = "LFS";

int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {

  struct esp_littlefs_t* efs = c->context;

  size_t part_off = (block * c->block_size) + off;

  esp_err_t err = esp_partition_read(efs->partition, part_off, buffer, size);
  if (err) {
    ESP_LOGE(TAG, "failed to read addr %08x, size %08x, err %d", (unsigned int)part_off, (unsigned int)size, err);
    return LFS_ERR_IO;
  }

  return 0;
}

int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {

  struct esp_littlefs_t* efs = c->context;

  size_t part_off = (block * c->block_size) + off;

  esp_err_t err = esp_partition_write(efs->partition, part_off, buffer, size);
  if (err) {
    ESP_LOGE(TAG, "failed to write addr %08x, size %08x, err %d", (unsigned int)part_off, (unsigned int)size, err);
    return LFS_ERR_IO;
  }

  return 0;
}

int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block) {

  struct esp_littlefs_t* efs = c->context;

  size_t part_off = block * c->block_size;

  esp_err_t err = esp_partition_erase_range(efs->partition, part_off, c->block_size);
  if (err) {
    ESP_LOGE(TAG, "failed to erase addr %08x, size %08x, err %d", (unsigned int)part_off, (unsigned int)c->block_size, err);
    return LFS_ERR_IO;
  }

  return 0;
}

int littlefs_api_sync(const struct lfs_config* c) {

  // Unnecessary for esp-idf
  return 0;
}

const char* littlefs_errno(enum lfs_error lfs_errno) {

  switch (lfs_errno) {
  case LFS_ERR_OK:
    return "LFS_ERR_OK";
  case LFS_ERR_IO:
    return "LFS_ERR_IO";
  case LFS_ERR_CORRUPT:
    return "LFS_ERR_CORRUPT";
  case LFS_ERR_NOENT:
    return "LFS_ERR_NOENT";
  case LFS_ERR_EXIST:
    return "LFS_ERR_EXIST";
  case LFS_ERR_NOTDIR:
    return "LFS_ERR_NOTDIR";
  case LFS_ERR_ISDIR:
    return "LFS_ERR_ISDIR";
  case LFS_ERR_NOTEMPTY:
    return "LFS_ERR_NOTEMPTY";
  case LFS_ERR_BADF:
    return "LFS_ERR_BADF";
  case LFS_ERR_FBIG:
    return "LFS_ERR_FBIG";
  case LFS_ERR_INVAL:
    return "LFS_ERR_INVAL";
  case LFS_ERR_NOSPC:
    return "LFS_ERR_NOSPC";
  case LFS_ERR_NOMEM:
    return "LFS_ERR_NOMEM";
  case LFS_ERR_NOATTR:
    return "LFS_ERR_NOATTR";
  case LFS_ERR_NAMETOOLONG:
    return "LFS_ERR_NAMETOOLONG";
  default:
    return "LFS_ERR_UNDEFINED";
  }

  return "";
}

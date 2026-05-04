#ifndef ESP_LITTLEFS_API_H
#define ESP_LITTLEFS_API_H

#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lfs.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LFS_PATH_MAX 15

struct esp_littlefs_t {

  lfs_t* lfs;
  struct lfs_config cfg;
  const esp_partition_t* partition;
  char base_path[ESP_LFS_PATH_MAX + 1];
  bool read_only;
};

/**
 * @brief Read a region in a block.
 *
 * Negative error codes are propagated to the user.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

/**
 * @brief Program a region in a block.
 *
 * The block must have previously been erased.
 * Negative error codes are propagated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

/**
 * @brief Erase a block.
 *
 * A block must be erased before being programmed.
 * The state of an erased block is undefined.
 * Negative error codes are propagated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 * @return errorcode. 0 on success.
 */
int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block);

/**
 * @brief Sync the state of the underlying block device.
 *
 * Negative error codes are propagated to the user.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_sync(const struct lfs_config* c);

#ifdef __cplusplus
}
#endif

#endif /* ESP_LITTLEFS_API_H */

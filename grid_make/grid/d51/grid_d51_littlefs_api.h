#ifndef GRID_D51_LITTLEFS_API_H
#define GRID_D51_LITTLEFS_API_H

#include "lfs.h"

enum { GRID_D51_LITTLEFS_PAGE_SIZE = 512 };
// enum { GRID_D51_LITTLEFS_PAGE_OFFSET = 0x100 };
enum { GRID_D51_LITTLEFS_BLOCK_SIZE = 8192 };

enum { GRID_D51_NVM_BASE_ADDR = 0x80000 };
enum { GRID_D51_NVM_END_ADDR = 0x100000 };
enum { GRID_D51_NVM_BYTES = GRID_D51_NVM_END_ADDR - GRID_D51_NVM_BASE_ADDR };
enum { GRID_D51_NVM_BLOCKS = GRID_D51_NVM_BYTES / GRID_D51_LITTLEFS_BLOCK_SIZE };

#define D51_LFS_PATH_MAX 15

struct d51_littlefs_t {

  lfs_t* lfs;
  struct lfs_config cfg;
  struct flash_descriptor* flash;
  char base_path[D51_LFS_PATH_MAX + 1];
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

#endif /* GRID_D51_LITTLEFS_API_H */

#ifndef GRID_RP2350_LITTLEFS_API_H
#define GRID_RP2350_LITTLEFS_API_H

#include <stdbool.h>

#include "littlefs/lfs.h"

// Flash geometry for the LittleFS partition on the RP2354 (2 MB internal
// stacked flash). The filesystem lives at the TOP of flash so the program
// image, which grows from offset 0, never collides with it.
//
// All offsets here are relative to the start of flash, which is what the Pico
// SDK flash_range_*() API expects. XIP_BASE is added only when reading through
// the memory-mapped window.
enum {
  GRID_RP2350_FLASH_SIZE = 0x200000,                                           // RP2354: 2 MB internal stacked flash
  GRID_RP2350_FS_SIZE = 0x80000,                                               // 512 KB reserved for LittleFS
  GRID_RP2350_FS_BASE = GRID_RP2350_FLASH_SIZE - GRID_RP2350_FS_SIZE,          // 0x180000
  GRID_RP2350_FS_BLOCK_SIZE = 4096,                                            // FLASH_SECTOR_SIZE (erase granularity)
  GRID_RP2350_FS_PAGE_SIZE = 256,                                              // FLASH_PAGE_SIZE (program granularity)
  GRID_RP2350_FS_BLOCK_COUNT = GRID_RP2350_FS_SIZE / GRID_RP2350_FS_BLOCK_SIZE // 128
};

#define RP2350_LFS_PATH_MAX 15

struct rp2350_littlefs_t {

  lfs_t* lfs;
  struct lfs_config cfg;
  char base_path[RP2350_LFS_PATH_MAX + 1];
  bool read_only;
};

/**
 * @brief Read a region in a block.
 *
 * Served directly from the memory-mapped XIP window, so no SDK call and no
 * interrupt masking is required.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

/**
 * @brief Program a region in a block.
 *
 * The block must have previously been erased. Interrupts are masked (via a
 * BASEPRI threshold, not a blanket disable) for the duration because XIP is
 * disabled while the flash is written, and any flash-resident code that runs
 * meanwhile would fault -- see the .c file's comment for which interrupt is
 * deliberately left unmasked and why.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

/**
 * @brief Erase a block.
 *
 * A block must be erased before being programmed. Interrupts are masked for the
 * same reason as littlefs_api_prog().
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block);

/**
 * @brief Sync the state of the underlying block device.
 *
 * No-op: writes are committed synchronously by littlefs_api_prog/erase.
 *
 * @return errorcode. 0 on success.
 */
int littlefs_api_sync(const struct lfs_config* c);

#endif /* GRID_RP2350_LITTLEFS_API_H */

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

// Served directly from the memory-mapped XIP window -- no SDK call or
// interrupt masking required.
int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

// Interrupts are masked (BASEPRI threshold) during the write, since XIP is
// disabled and flash-resident code would fault; see .c for the exception.
int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

// Interrupts are masked for the same reason as littlefs_api_prog().
int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block);

// No-op: writes are committed synchronously by prog/erase.
int littlefs_api_sync(const struct lfs_config* c);

#endif /* GRID_RP2350_LITTLEFS_API_H */

#ifndef GRID_RP2350_LITTLEFS_API_H
#define GRID_RP2350_LITTLEFS_API_H

#include <stdbool.h>

#include "littlefs/lfs.h"

enum { GRID_RP2350_FLASH_SIZE = 0x200000 };
enum { GRID_RP2350_FS_SIZE = 0x80000 };
enum { GRID_RP2350_FS_BASE = GRID_RP2350_FLASH_SIZE - GRID_RP2350_FS_SIZE };
enum { GRID_RP2350_FS_PAGE_SIZE = 256 };
enum { GRID_RP2350_FS_BLOCK_SIZE = 4096 };

#define RP2350_LFS_PATH_MAX 15

struct rp2350_littlefs_t {

  lfs_t* lfs;
  struct lfs_config cfg;
  char base_path[RP2350_LFS_PATH_MAX + 1];
  bool read_only;
};

int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block);

int littlefs_api_sync(const struct lfs_config* c);

#endif /* GRID_RP2350_LITTLEFS_API_H */

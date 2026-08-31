#include "grid_rp2350_littlefs.h"

#include <assert.h>
#include <string.h>

#include "grid_littlefs.h"

void grid_rp2350_littlefs_init(struct rp2350_littlefs_t* rfs, const char* base_path, bool read_only) {

  struct lfs_config lfs_cfg = (struct lfs_config){

      .context = rfs,

      // Block device operations
      .read = littlefs_api_read,
      .prog = littlefs_api_prog,
      .erase = littlefs_api_erase,
      .sync = littlefs_api_sync,

      // Block device configuration
      .read_size = GRID_RP2350_FS_PAGE_SIZE,
      .prog_size = GRID_RP2350_FS_PAGE_SIZE,
      .block_size = GRID_RP2350_FS_BLOCK_SIZE,
      .block_count = GRID_RP2350_FS_BLOCK_COUNT,
      .cache_size = GRID_RP2350_FS_PAGE_SIZE,
      .lookahead_size = 32,
      .block_cycles = 500,
  };

  {
    assert(!rfs->lfs);
    rfs->cfg = lfs_cfg;
    rfs->read_only = read_only;
  }

  strcpy(rfs->base_path, base_path);
  assert(base_path[0] == '\0');
}

int grid_rp2350_littlefs_mount(struct rp2350_littlefs_t* rfs, bool force_format) {

  grid_rp2350_littlefs_init(rfs, "", false);

  rfs->lfs = grid_littlefs_mount(&rfs->cfg, force_format);
  return rfs->lfs ? 0 : 1;
}

int grid_rp2350_littlefs_unmount(struct rp2350_littlefs_t* rfs) { return grid_littlefs_unmount(&rfs->lfs); }

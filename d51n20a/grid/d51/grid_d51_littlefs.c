#include "grid_d51_littlefs.h"

#include "grid_littlefs.h"

void grid_d51_littlefs_init(struct d51_littlefs_t* dfs, struct flash_descriptor* flash, const char* base_path, bool read_only) {

  struct lfs_config lfs_cfg = (struct lfs_config){

      .context = dfs,

      // Block device operations
      .read = littlefs_api_read,
      .prog = littlefs_api_prog,
      .erase = littlefs_api_erase,
      .sync = littlefs_api_sync,

      // Block device configuration
      .read_size = 128,
      .prog_size = 128,
      .block_size = GRID_D51_LITTLEFS_BLOCK_SIZE,
      .block_count = GRID_D51_NVM_BLOCKS,
      .cache_size = 512,
      .lookahead_size = 128,
      .block_cycles = 512,
      // .disk-version = 0x00020001,
  };

  {
    assert(!dfs->lfs);
    dfs->cfg = lfs_cfg;
    dfs->flash = flash;
    dfs->read_only = read_only;
  }

  strcpy(dfs->base_path, base_path);
  assert(base_path[0] == '\0');
}

int grid_d51_littlefs_mount(struct d51_littlefs_t* dfs, bool force_format) {

  // Check that the littlefs page size is evenly divisible by the flash chip page size
  if (GRID_D51_LITTLEFS_PAGE_SIZE % flash_get_page_size(&FLASH_0)) {
    printf("littlefs page size not evenly divisible by flash chip page size\n");
    return 1;
  }

  grid_d51_littlefs_init(dfs, &FLASH_0, "", false);

  dfs->lfs = grid_littlefs_mount(&dfs->cfg, force_format);
  return dfs->lfs ? 0 : 1;
}

int grid_d51_littlefs_unmount(struct d51_littlefs_t* dfs) { return grid_littlefs_unmount(&dfs->lfs); }

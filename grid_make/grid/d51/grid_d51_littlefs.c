#include "grid_d51_littlefs.h"

// #include "../atmel_start.h"
// #include "../samd51a/include/samd51n20a.h"
// #include "atmel_start_pins.h"

#include "grid_d51_littlefs_api.h"

void grid_d51_littlefs_init(struct d51_littlefs_t* dfs, lfs_t* lfs, struct flash_descriptor* flash, const char* base_path, bool read_only) {

  struct lfs_config lfs_cfg = (struct lfs_config){

      .context = dfs,

      // Block device operations
      .read = littlefs_api_read,
      .prog = littlefs_api_prog,
      .erase = littlefs_api_erase,
      .sync = littlefs_api_sync,

      // Block device configuration
  };

  {
    dfs->lfs = lfs;
    dfs->cfg = lfs_cfg;
    dfs->flash = flash;
    dfs->read_only = read_only;
  }

  strcpy(dfs->base_path, base_path);
}

int grid_d51_littlefs_mount(struct d51_littlefs_t* dfs) {

  // Allocate littlefs
  lfs_t* lfs = malloc(sizeof(lfs_t));
  if (!lfs) {
    printf("failed to allocate littlefs\n");
    return 1;
  }

  grid_d51_littlefs_init(dfs, lfs, &FLASH_0, "/littlefs", false);

  if (grid_littlefs_mount_or_format(dfs->lfs, &dfs->cfg)) {
    return 1;
  }

  if (grid_littlefs_mkdir_base(dfs->lfs, dfs->base_path)) {
    return 1;
  }

  return 0;
}

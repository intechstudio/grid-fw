/*
 * grid_nvm.c
 *
 * Created: 9/15/2020 3:41:44 PM
 *  Author: suku
 */

#include "grid_d51_nvm.h"

#include "grid_platform.h"

#include "grid_d51_littlefs.h"
#include "grid_littlefs.h"

struct grid_d51_nvm_model grid_d51_nvm_state;

void grid_d51_nvm_mount(struct grid_d51_nvm_model* nvm, bool force_format) {

  // Initialize and mount littlefs
  int ret = grid_d51_littlefs_mount(&nvm->dfs, force_format);

  if (ret) {
    printf("failed to initialize littlefs\n");
    return;
  }

  // Set pointer to littlefs
  grid_platform_set_lfs(nvm->dfs.lfs);

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->dfs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->dfs.lfs, &nvm->dfs.cfg);
  printf("littlefs size: total: %d, used: %d\n", total, used);

  // List the filesystem root
  grid_platform_list_directory("");
}

void grid_d51_nvm_unmount(struct grid_d51_nvm_model* nvm) {

  // Unmount littlefs
  int ret = grid_d51_littlefs_unmount(&nvm->dfs);

  if (ret) {
    printf("failed to deinitialize littlefs\n");
    return;
  }
}

void grid_platform_nvm_erase() {

  grid_platform_delete_actionstring_files_all();

  struct grid_file_t handle = {0};
  if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &handle) == 0) {
    grid_platform_delete_file(&handle);
  }
}

void grid_platform_nvm_format_and_mount() {

  grid_d51_nvm_unmount(&grid_d51_nvm_state);
  grid_d51_nvm_mount(&grid_d51_nvm_state, true);
}

const char* grid_platform_get_base_path() { return grid_d51_nvm_state.dfs.base_path; }

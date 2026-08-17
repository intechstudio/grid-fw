#include "grid_rp2350_nvm.h"

#include <stdio.h>

#include "grid_platform.h"

#include "grid_littlefs.h"
#include "grid_rp2350_littlefs.h"

struct grid_rp2350_nvm_model grid_rp2350_nvm_state;

void grid_rp2350_nvm_mount(struct grid_rp2350_nvm_model* nvm, bool force_format) {

  // Initialize and mount littlefs
  int ret = grid_rp2350_littlefs_mount(&nvm->rfs, force_format);

  if (ret) {
    printf("failed to initialize littlefs\n");
    return;
  }

  // Set pointer to littlefs
  grid_platform_set_lfs(nvm->rfs.lfs);

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->rfs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->rfs.lfs, &nvm->rfs.cfg);
  printf("littlefs size: total: %d, used: %d\n", (int)total, (int)used);

  // List the filesystem root
  grid_platform_lsdir("");
}

void grid_rp2350_nvm_unmount(struct grid_rp2350_nvm_model* nvm) {

  // Unmount littlefs
  int ret = grid_rp2350_littlefs_unmount(&nvm->rfs);

  if (ret) {
    printf("failed to deinitialize littlefs\n");
    return;
  }
}

void grid_platform_nvm_format_and_mount() {

  grid_rp2350_nvm_unmount(&grid_rp2350_nvm_state);
  grid_rp2350_nvm_mount(&grid_rp2350_nvm_state, true);
}

const char* grid_platform_get_base_path() { return grid_rp2350_nvm_state.rfs.base_path; }

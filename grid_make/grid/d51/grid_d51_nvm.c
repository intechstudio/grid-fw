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

void grid_d51_nvm_mount(struct grid_d51_nvm_model* nvm) {

  // Initialize and mount littlefs
  int ret = grid_d51_littlefs_mount(&nvm->dfs);

  if (ret) {
    printf("failed to initialize littlefs\n");
    return;
  }

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->dfs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->dfs.lfs, &nvm->dfs.cfg);
  printf("littlefs size: total: %d, used: %d\n", total, used);

  // List the filesystem root
  grid_platform_list_directory("");
}

#define LFS grid_d51_nvm_state.dfs.lfs

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, struct grid_file_t* handle) {

  int ret = grid_littlefs_find_next_on_page(LFS, page, last_element, last_event);

  if (ret == 0) {

    sprintf(handle->path, "%02x/%02x/%02x.cfg", page, *last_element, *last_event);
  }

  return ret;
}

int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event, struct grid_file_t* handle) {

  char path[50] = {0};
  sprintf(path, "%02x/%02x/%02x.cfg", page, element, event);

  return grid_platform_find_file(path, handle);
}

int grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event, char* buffer, uint16_t length) {

  char path[50] = {0};

  sprintf(&path[strlen(path)], "%02x", page);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x", element);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x.cfg", event);
  return grid_platform_write_file(path, (uint8_t*)buffer, length + 1);
}

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page) {

  char path[50] = {0};

  // upkeep: loop bound
  for (uint8_t i = 0; i < 18 + 1; ++i) {

    // Remove element directory
    sprintf(path, "%02x/%02x", page, i);
    grid_platform_remove_dir(path);
  }

  // Remove page directory
  sprintf(path, "%02x", page);
  grid_platform_remove_dir(path);
}

void grid_platform_delete_actionstring_files_all() {

  // upkeep: loop bound
  for (uint8_t i = 0; i < 4; ++i) {

    grid_platform_clear_all_actionstring_files_from_page(i);
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

  struct d51_littlefs_t* dfs = &grid_d51_nvm_state.dfs;

  grid_littlefs_mount_or_format(dfs->lfs, &dfs->cfg, true);
  grid_littlefs_mkdir_base(dfs->lfs, dfs->base_path);
}

const char* grid_platform_get_base_path() { return grid_d51_nvm_state.dfs.base_path; }

int grid_platform_make_directory(const char* path) { return grid_littlefs_mkdir(LFS, path); }

int grid_platform_list_directory(const char* path) { return grid_littlefs_lsdir(LFS, path); }

int grid_platform_find_file(const char* path, struct grid_file_t* handle) {

  int status = grid_littlefs_file_find(LFS, path);

  if (status == 0) {
    strcpy(handle->path, path);
  }

  return status;
}

uint16_t grid_platform_get_file_size(struct grid_file_t* handle) { return grid_littlefs_file_size(LFS, handle->path); }

int grid_platform_read_file(struct grid_file_t* handle, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_read(LFS, handle->path, buffer, size); }

int grid_platform_write_file(const char* path, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_write(LFS, path, buffer, size); }

int grid_platform_delete_file(struct grid_file_t* handle) { return grid_littlefs_remove(LFS, handle->path); }

int grid_platform_remove_path(const char* path) { return grid_littlefs_remove(LFS, path); }

int grid_platform_remove_dir(const char* path) { return grid_littlefs_rmdir(LFS, path); }

uint8_t grid_platform_get_nvm_state() { return hri_nvmctrl_get_STATUS_READY_bit(grid_d51_nvm_state.dfs.flash->dev.hw); }

void grid_platform_nvm_defrag() {}

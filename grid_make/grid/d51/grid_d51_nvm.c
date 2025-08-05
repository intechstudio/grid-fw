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
    printf("failed to initialize littlefs"); // TODO error string
    return;
  }

  // Retrieve filesystem size information
  size_t total = grid_littlefs_get_total_bytes(&nvm->dfs.cfg);
  size_t used = grid_littlefs_get_used_bytes(nvm->dfs.lfs, &nvm->dfs.cfg);
  printf("littlefs size: total: %d, used: %d", total, used);

  // List the filesystem root
  grid_platform_list_directory("");
}

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle) {

  int ret = grid_littlefs_find_next_on_page(grid_d51_nvm_state.dfs.lfs, page, last_element, last_event);

  if (ret == 0) {

    sprintf(file_handle->fname, "%02x/%02x/%02x.cfg", page, *last_element, *last_event);
  }

  return ret;
}

int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event, union grid_ui_file_handle* file_handle) {

  char path[50] = {0};
  sprintf(path, "%02x/%02x/%02x.cfg", page, element, event);

  int status = grid_platform_find_file(path, file_handle);

  // Clear file handle if not found
  if (status) {
    memset(file_handle, 0, sizeof(union grid_ui_file_handle));
  }

  return status;
}

void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle) { grid_platform_delete_file(file_handle); }

uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle) { return grid_platform_get_file_size(file_handle); }

uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring, uint16_t size) {

  return grid_platform_read_file(file_handle, (uint8_t*)targetstring, size);
}

void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event, char* buffer, uint16_t length) {

  char path[50] = {0};

  sprintf(&path[strlen(path)], "%02x", page);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x", element);
  grid_platform_make_directory(path);

  sprintf(&path[strlen(path)], "/%02x.cfg", event);
  int ret = grid_platform_write_file(path, (uint8_t*)buffer, length + 1);
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

  union grid_ui_file_handle file_handle = {0};
  if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &file_handle) == 0) {
    grid_platform_delete_file(&file_handle);
  }
}

const char* grid_platform_get_base_path() { return grid_d51_nvm_state.dfs.base_path; }

int grid_platform_make_directory(const char* path) { return grid_littlefs_mkdir(grid_d51_nvm_state.dfs.lfs, path); }

int grid_platform_list_directory(const char* path) { return grid_littlefs_lsdir(grid_d51_nvm_state.dfs.lfs, path); }

int grid_platform_find_file(const char* path, union grid_ui_file_handle* file_handle) {

  int status = grid_littlefs_file_find(grid_d51_nvm_state.dfs.lfs, path);

  if (status == 0) {
    strcpy(file_handle->fname, path);
  }

  return status;
}

uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle) { return grid_littlefs_file_size(grid_d51_nvm_state.dfs.lfs, file_handle->fname); }

int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_read(grid_d51_nvm_state.dfs.lfs, file_handle->fname, buffer, size); }

int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size) { return grid_littlefs_file_write(grid_d51_nvm_state.dfs.lfs, path, buffer, size); }

int grid_platform_delete_file(union grid_ui_file_handle* file_handle) { return grid_littlefs_remove(grid_d51_nvm_state.dfs.lfs, file_handle->fname); }

int grid_platform_remove_path(const char* path) { return grid_littlefs_remove(grid_d51_nvm_state.dfs.lfs, path); }

int grid_platform_remove_dir(const char* path) { return grid_littlefs_rmdir(grid_d51_nvm_state.dfs.lfs, path); }

uint8_t grid_platform_get_nvm_state() { return hri_nvmctrl_get_STATUS_READY_bit(grid_d51_nvm_state.dfs.flash->dev.hw); }

// TODO
uint8_t grid_platform_erase_nvm_next() { return 1; }

// TODO
uint32_t grid_plaform_get_nvm_nextwriteoffset() { return 0; }

// TODO
void grid_platform_nvm_defrag() {}

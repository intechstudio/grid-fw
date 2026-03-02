#include "grid_platform.h"

#include <assert.h>

#include "grid_littlefs.h"

static void* LFS = NULL;

void grid_platform_set_lfs(void* lfs) { LFS = lfs; }

int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, struct grid_file_t* handle) {

  assert(LFS);

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

int grid_platform_make_directory(const char* path) {

  assert(LFS);

  return grid_littlefs_mkdir(LFS, path);
}

int grid_platform_list_directory(const char* path) {

  assert(LFS);

  return grid_littlefs_lsdir(LFS, path);
}

int grid_platform_find_file(const char* path, struct grid_file_t* handle) {

  assert(LFS);

  int status = grid_littlefs_file_find(LFS, path);

  if (status == 0) {
    strcpy(handle->path, path);
  }

  return status;
}

uint16_t grid_platform_get_file_size(struct grid_file_t* handle) {

  assert(LFS);

  return grid_littlefs_file_size(LFS, handle->path);
}

int grid_platform_read_file(struct grid_file_t* handle, uint8_t* buffer, uint16_t size) {

  assert(LFS);

  return grid_littlefs_file_read(LFS, handle->path, buffer, size);
}

int grid_platform_write_file(const char* path, uint8_t* buffer, uint16_t size) {

  assert(LFS);

  return grid_littlefs_file_write(LFS, path, buffer, size);
}

int grid_platform_delete_file(struct grid_file_t* handle) {

  assert(LFS);

  return grid_littlefs_remove(LFS, handle->path);
}

int grid_platform_remove_path(const char* path) {

  assert(LFS);

  return grid_littlefs_remove(LFS, path);
}

int grid_platform_remove_dir(const char* path) {

  assert(LFS);

  return grid_littlefs_rmdir(LFS, path);
}

uint8_t grid_platform_get_nvm_state() { return LFS != NULL; }

void grid_platform_nvm_defrag() {}

void* grid_platform_fopen(const char* pathname, const char* mode) {

  assert(LFS);

  return grid_littlefs_fopen(LFS, pathname, mode);
}

int grid_platform_fclose(void* stream) {

  assert(LFS);

  return grid_littlefs_fclose(LFS, stream);
}

size_t grid_platform_fwrite(const void* ptr, size_t size, size_t nmemb, void* stream) {

  assert(LFS);

  if (stream == stdin || stream == stderr) {
    return 0;
  }

  if (stream == stdout) {
    return fwrite(ptr, size, nmemb, stream);
  }

  return grid_littlefs_fwrite(LFS, ptr, size, nmemb, stream);
}

size_t grid_platform_fread(void* ptr, size_t size, size_t nmemb, void* stream) {

  assert(LFS);

  if (stream == stdin || stream == stdout || stream == stderr) {
    return 0;
  }

  return grid_littlefs_fread(LFS, ptr, size, nmemb, stream);
}

long grid_platform_ftell(void* stream) {

  assert(LFS);

  return grid_littlefs_ftell(LFS, stream);
}

int grid_platform_fseek(void* stream, long offset, int whence) {

  assert(LFS);

  return grid_littlefs_fseek(LFS, stream, offset, whence);
}

void grid_platform_clearerr(void* stream) {}

int grid_platform_ferror(void* stream) { return 0; }

int grid_platform_getc(void* stream) {

  assert(LFS);

  if (stream == stdin || stream == stdout || stream == stderr) {
    return EOF;
  }

  unsigned char c;

  return grid_littlefs_fread(LFS, &c, 1, 1, stream) == 1 ? c : EOF;
}

int grid_platform_ungetc(int c, void* stream) {

  assert(LFS);

  if (stream == stdin || stream == stdout || stream == stderr) {
    return EOF;
  }

  lfs_soff_t soff = grid_littlefs_fseek(LFS, stream, -1, SEEK_CUR);
  if (soff < 0) {
    return EOF;
  }

  return c;
}

int grid_platform_fflush(void* stream) {

  assert(LFS);

  if (stream == stdin || stream == stderr) {
    return EOF;
  }

  if (stream == stdout) {
    return fflush(stdout);
  }

  return grid_littlefs_fflush(LFS, stream);
}

int grid_platform_remove(const char* pathname) {

  assert(LFS);

  return grid_littlefs_remove(LFS, pathname) == 0 ? 0 : -1;
}

void* grid_platform_opendir(const char* name) {

  assert(LFS);

  return grid_littlefs_opendir(LFS, name);
}

int grid_platform_closedir(void* dirp) {

  assert(LFS);

  return grid_littlefs_closedir(LFS, dirp);
}

void* grid_platform_readdir(void* dirp) {

  assert(LFS);

  return grid_littlefs_readdir(LFS, dirp);
}

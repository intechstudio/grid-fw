#include "grid_platform.h"

#include <assert.h>

#include "grid_littlefs.h"

static void* LFS = NULL;

void grid_platform_set_lfs(void* lfs) { LFS = lfs; }

char* grid_platform_read_file_contents(const char* path) {

  void* statbuf;
  int status = grid_platform_stat(path, &statbuf);
  if (status) {
    return NULL;
  }

  size_t size = grid_platform_file_info_size(statbuf);
  if (size == 0) {
    return NULL;
  }

  char* buf = malloc(size + 1);
  if (!buf) {
    return NULL;
  }

  void* file = grid_platform_fopen(path, "r");
  if (!file) {
    free(buf);
    return NULL;
  }

  if (grid_platform_fread(buf, size, 1, file) != 1) {
    free(buf);
    grid_platform_fclose(file);
    return NULL;
  }

  buf[size] = '\0';

  if (grid_platform_fclose(file)) {
    free(buf);
    return NULL;
  }

  return buf;
}

int grid_platform_write_file_contents(const char* buf, const char* path) {

  void* file = grid_platform_fopen(path, "w");
  if (!file) {
    return 1;
  }

  if (grid_platform_fwrite(buf, strlen(buf), 1, file) != 1) {
    grid_platform_fclose(file);
    return 1;
  }

  return grid_platform_fclose(file);
}

void* grid_platform_dir_first(const char* path) {

  assert(LFS);

  return grid_littlefs_dir_first(LFS, path);
}

void grid_platform_delete_script_files_all() {

  // upkeep: loop bound
  for (uint8_t i = 0; i < 4; ++i) {

    char path[3] = {0};
    snprintf(path, 3, "%02x", i);

    // Remove page directory, assuming remove is recursive
    grid_platform_remove(path);
  }
}

void grid_platform_nvm_erase() {

  grid_platform_delete_script_files_all();

  void* dummy;
  if (grid_platform_stat(GRID_UI_CONFIG_PATH, &dummy) == 0) {
    grid_platform_remove(GRID_UI_CONFIG_PATH);
  }
}

int grid_platform_mkdir(const char* path) {

  assert(LFS);

  return grid_littlefs_mkdir(LFS, path);
}

int grid_platform_lsdir(const char* path) {

  assert(LFS);

  return grid_littlefs_lsdir(LFS, path);
}

int grid_platform_stat(const char* path, void** statbuf) {

  assert(LFS);

  return grid_littlefs_stat(LFS, path, statbuf);
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

  return grid_littlefs_remove(LFS, pathname);
}

int grid_platform_rename(const char* oldpath, const char* newpath) {

  assert(LFS);

  return grid_littlefs_rename(LFS, oldpath, newpath);
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

const char* grid_platform_file_info_name(void* info) { return grid_littlefs_file_info_name(info); }

uint8_t grid_platform_file_info_type(void* info) { return grid_littlefs_file_info_type(info); }

uint32_t grid_platform_file_info_size(void* info) { return grid_littlefs_file_info_size(info); }

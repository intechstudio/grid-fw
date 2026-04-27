#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

void* grid_platform_fopen(const char* pathname, const char* mode) { return NULL; }

int grid_platform_fclose(void* stream) { return 0; }

size_t grid_platform_fwrite(const void* ptr, size_t size, size_t nmemb, void* stream) { return 0; }

size_t grid_platform_fread(void* ptr, size_t size, size_t nmemb, void* stream) { return 0; }

long grid_platform_ftell(void* stream) { return 0; }

int grid_platform_fseek(void* stream, long offset, int whence) { return 0; }

void grid_platform_clearerr(void* stream) {}

int grid_platform_ferror(void* stream) { return 0; }

int grid_platform_getc(void* stream) { return 0; }

int grid_platform_ungetc(int c, void* stream) { return 0; }

int grid_platform_fflush(void* stream) { return 0; }

int grid_platform_remove(const char* pathname) { return 0; }

int grid_platform_rename(const char* oldpath, const char* newpath) { return 0; }

void* grid_platform_opendir(const char* name) { return NULL; }

int grid_platform_closedir(void* dirp) { return 0; }

void* grid_platform_readdir(void* dirp) { return NULL; }

const char* grid_platform_file_info_name(void* info) { return NULL; }

uint8_t grid_platform_file_info_type(void* info) { return 0; }

uint32_t grid_platform_file_info_size(void* info) { return 0; }

void grid_platform_delay_ms(uint32_t delay_milliseconds) { return; }

int grid_platform_mkdir(const char* path) { return 0; }

int grid_platform_stat(const char* path, void** statbuf) { return 0; }

int grid_platform_find_file(const char* path, struct grid_file_t* handle) { return 0; }

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

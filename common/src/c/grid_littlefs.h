#ifndef GRID_LITTLEFS_H
#define GRID_LITTLEFS_H

#include "littlefs/lfs.h"

const char* littlefs_errno(enum lfs_error lfs_errno);

int grid_littlefs_mount_or_format(lfs_t* lfs, struct lfs_config* cfg, bool force_format);
int grid_littlefs_unmount(lfs_t* lfs);
int grid_littlefs_mkdir_base(lfs_t* lfs, const char* path);

int grid_littlefs_path_build(const char* path, uint16_t out_size, char* out);

int grid_littlefs_remove(lfs_t* lfs, const char* path);

int grid_littlefs_mkdir(lfs_t* lfs, const char* path);
int grid_littlefs_rmdir(lfs_t* lfs, const char* path);
int grid_littlefs_lsdir(lfs_t* lfs, const char* path);

int grid_littlefs_file_find(lfs_t* lfs, const char* path);
size_t grid_littlefs_file_size(lfs_t* lfs, const char* path);
int grid_littlefs_file_read(lfs_t* lfs, const char* path, uint8_t* buffer, uint16_t size);
int grid_littlefs_file_write(lfs_t* lfs, const char* path, const uint8_t* buffer, uint16_t size);

size_t grid_littlefs_get_total_bytes(struct lfs_config* cfg);
size_t grid_littlefs_get_used_bytes(lfs_t* lfs, struct lfs_config* cfg);

int grid_littlefs_find_next_on_page(lfs_t* lfs, uint8_t page, int* last_ele, int* last_evt);

lfs_file_t* grid_littlefs_fopen(lfs_t* lfs, const char* path, const char* mode);
int grid_littlefs_fclose(lfs_t* lfs, lfs_file_t* stream);
size_t grid_littlefs_fwrite(lfs_t* lfs, const void* ptr, size_t size, size_t nmemb, lfs_file_t* stream);
size_t grid_littlefs_fread(lfs_t* lfs, void* ptr, size_t size, size_t nmemb, lfs_file_t* stream);
ssize_t grid_littlefs_ftell(lfs_t* lfs, lfs_file_t* stream);
ssize_t grid_littlefs_fseek(lfs_t* lfs, lfs_file_t* stream, lfs_soff_t soff, int whence);
int grid_littlefs_fflush(lfs_t* lfs, lfs_file_t* stream);
lfs_dir_t* grid_littlefs_opendir(lfs_t* lfs, const char* name);
int grid_littlefs_closedir(lfs_t* lfs, lfs_dir_t* dirp);
char* grid_littlefs_readdir(lfs_t* lfs, lfs_dir_t* dirp);

#endif /* GRID_LITTLEFS_H */

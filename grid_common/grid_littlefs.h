#ifndef GRID_LITTLEFS_H
#define GRID_LITTLEFS_H

#include "littlefs/lfs.h"

typedef bool (*grid_littlefs_name_predicate)(const char* name, const void* user);

const char* littlefs_errno(enum lfs_error lfs_errno);

int grid_littlefs_mount_or_format(lfs_t* lfs, struct lfs_config* cfg);
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

#endif /* GRID_LITTLEFS_H */

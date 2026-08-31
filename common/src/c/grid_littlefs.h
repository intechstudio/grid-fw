#ifndef GRID_LITTLEFS_H
#define GRID_LITTLEFS_H

#include "littlefs/lfs.h"

const char* littlefs_errno(enum lfs_error lfs_errno);

int grid_littlefs_mount_or_format(lfs_t* lfs, struct lfs_config* cfg, bool force_format);

// Allocates an lfs_t and mounts (or formats-then-mounts) it via
// grid_littlefs_mount_or_format, returning the mounted lfs_t on success or
// NULL on failure (freeing the allocation itself, never leaving a dangling
// pointer for the caller to mishandle) -- owns the malloc/free bookkeeping
// that D51, ESP32, and RP2350 each used to duplicate (and each independently
// fix the same dangling-pointer bug in).
lfs_t* grid_littlefs_mount(struct lfs_config* cfg, bool force_format);

// Unmounts and frees *lfs (asserted non-NULL on entry), leaving it NULL only
// on success -- mirrors grid_littlefs_mount's ownership (the caller cannot
// end up with a dangling pointer), and folds in the same guard-ordering fix
// (assert before, not after, the unmount call) each platform's copy needed
// separately.
int grid_littlefs_unmount(lfs_t** lfs);

int grid_littlefs_remove(lfs_t* lfs, const char* path);
int grid_littlefs_rename(lfs_t* lfs, const char* oldpath, const char* newpath);

int grid_littlefs_mkdir(lfs_t* lfs, const char* path);
int grid_littlefs_rmdir(lfs_t* lfs, const char* path);
int grid_littlefs_lsdir(lfs_t* lfs, const char* path);

int grid_littlefs_stat(lfs_t* lfs, const char* path, void** statbuf);
void* grid_littlefs_dir_first(lfs_t* lfs, const char* path);

size_t grid_littlefs_get_total_bytes(struct lfs_config* cfg);
size_t grid_littlefs_get_used_bytes(lfs_t* lfs, struct lfs_config* cfg);

lfs_file_t* grid_littlefs_fopen(lfs_t* lfs, const char* path, const char* mode);
int grid_littlefs_fclose(lfs_t* lfs, lfs_file_t* stream);
size_t grid_littlefs_fwrite(lfs_t* lfs, const void* ptr, size_t size, size_t nmemb, lfs_file_t* stream);
size_t grid_littlefs_fread(lfs_t* lfs, void* ptr, size_t size, size_t nmemb, lfs_file_t* stream);
ssize_t grid_littlefs_ftell(lfs_t* lfs, lfs_file_t* stream);
ssize_t grid_littlefs_fseek(lfs_t* lfs, lfs_file_t* stream, lfs_soff_t soff, int whence);
int grid_littlefs_fflush(lfs_t* lfs, lfs_file_t* stream);
lfs_dir_t* grid_littlefs_opendir(lfs_t* lfs, const char* name);
int grid_littlefs_closedir(lfs_t* lfs, lfs_dir_t* dirp);
void* grid_littlefs_readdir(lfs_t* lfs, lfs_dir_t* dirp);

const char* grid_littlefs_file_info_name(struct lfs_info* info);
uint8_t grid_littlefs_file_info_type(struct lfs_info* info);
uint32_t grid_littlefs_file_info_size(struct lfs_info* info);

#endif /* GRID_LITTLEFS_H */

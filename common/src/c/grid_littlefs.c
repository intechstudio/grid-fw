#include "grid_littlefs.h"

#include <assert.h>
#include <limits.h>

#include "grid_platform.h"

const char* littlefs_errno(enum lfs_error lfs_errno) {

  switch (lfs_errno) {
  case LFS_ERR_OK:
    return "LFS_ERR_OK";
  case LFS_ERR_IO:
    return "LFS_ERR_IO";
  case LFS_ERR_CORRUPT:
    return "LFS_ERR_CORRUPT";
  case LFS_ERR_NOENT:
    return "LFS_ERR_NOENT";
  case LFS_ERR_EXIST:
    return "LFS_ERR_EXIST";
  case LFS_ERR_NOTDIR:
    return "LFS_ERR_NOTDIR";
  case LFS_ERR_ISDIR:
    return "LFS_ERR_ISDIR";
  case LFS_ERR_NOTEMPTY:
    return "LFS_ERR_NOTEMPTY";
  case LFS_ERR_BADF:
    return "LFS_ERR_BADF";
  case LFS_ERR_FBIG:
    return "LFS_ERR_FBIG";
  case LFS_ERR_INVAL:
    return "LFS_ERR_INVAL";
  case LFS_ERR_NOSPC:
    return "LFS_ERR_NOSPC";
  case LFS_ERR_NOMEM:
    return "LFS_ERR_NOMEM";
  case LFS_ERR_NOATTR:
    return "LFS_ERR_NOATTR";
  case LFS_ERR_NAMETOOLONG:
    return "LFS_ERR_NAMETOOLONG";
  default:
    return "LFS_ERR_UNDEFINED";
  }

  return "";
}

int grid_littlefs_mount_or_format(lfs_t* lfs, struct lfs_config* cfg, bool force_format) {

  // Mount littlefs
  int lfs_err = force_format ? LFS_ERR_CORRUPT : lfs_mount(lfs, cfg);

  // Attempt a format (then mount) if necessary
  if (lfs_err != LFS_ERR_OK) {

    if (force_format) {
      printf("littlefs force formatting...\n");
    } else {
      printf("littlefs mount failed (%d): %s. formatting...\n", lfs_err, littlefs_errno(lfs_err));
    }

    lfs_err = lfs_format(lfs, cfg);
    if (lfs_err != LFS_ERR_OK) {
      printf("littlefs format failed\n");
      return 1;
    }

    printf("littlefs format successful. mounting...\n");

    lfs_err = lfs_mount(lfs, cfg);
    if (lfs_err != LFS_ERR_OK) {
      printf("littlefs mount failed (%d): %s. exiting...\n", lfs_err, littlefs_errno(lfs_err));
      return 1;
    }
  }

  return 0;
}

int grid_littlefs_unmount(lfs_t* lfs) {

  // Unmount littlefs
  int lfs_err = lfs_unmount(lfs);
  if (lfs_err != LFS_ERR_OK) {
    printf("littlefs unmount failed (%d): %s. exiting...\n", lfs_err, littlefs_errno(lfs_err));
    return 1;
  }

  return 0;
}

int grid_littlefs_remove(lfs_t* lfs, const char* path) {

  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, path, &info);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_remove stat error: %d\n", lfs_err);
    return 1;
  }

  switch (info.type) {
  case LFS_TYPE_REG: {

    int lfs_err = lfs_remove(lfs, path);
    if (lfs_err != LFS_ERR_OK) {
      printf("grid_littlefs_remove error: %d\n", lfs_err);
      return 1;
    }

    return 0;

  } break;
  case LFS_TYPE_DIR: {

    return grid_littlefs_rmdir(lfs, path);

  } break;
  default: {

    printf("grid_littlefs_remove unknown type: %d\n", info.type);
    return 1;
  }
  }

  assert(0); // unreachable
  return 0;
}

int grid_littlefs_rename(lfs_t* lfs, const char* oldpath, const char* newpath) {

  int lfs_err = lfs_rename(lfs, oldpath, newpath);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_rename error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_mkdir(lfs_t* lfs, const char* path) {

  int lfs_err = lfs_mkdir(lfs, path);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_mkdir error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_rmdir(lfs_t* lfs, const char* path) {

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, path);
  if (lfs_err != LFS_ERR_OK) {
    // printf("grid_littlefs_rmdir open error: %d\n", lfs_err);
    return 1;
  }

  struct lfs_info info;
  while (lfs_dir_read(lfs, &dir, &info) > 0) {

    // Ignore entry for current directory
    if (strcmp(info.name, ".") == 0) {
      continue;
    }

    // Ignore entry for parent directory
    if (strcmp(info.name, "..") == 0) {
      continue;
    }

    char path2[LFS_NAME_MAX * 2 + 2] = {0};
    sprintf(path2, "%s/%s", path, info.name);

    int lfs_err = grid_littlefs_remove(lfs, path2);
    if (lfs_err != LFS_ERR_OK) {
      printf("grid_littlefs_rmdir remove entry error %d\n", lfs_err);
      return lfs_err;
    }
  }

  lfs_err = lfs_dir_close(lfs, &dir);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_rmdir close error: %d\n", lfs_err);
    return 1;
  }

  lfs_err = lfs_remove(lfs, path);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_rmdir remove error %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_lsdir(lfs_t* lfs, const char* path) {

  printf("list directory: \"%s\"\n", path);

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, path);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_lsdir open error: %d\n", lfs_err);
    return 1;
  }

  struct lfs_info info;
  while (lfs_dir_read(lfs, &dir, &info) > 0) {
    printf("type: %d, size: %8lu, name: %s\n", info.type, info.size, info.name);
  }

  lfs_err = lfs_dir_close(lfs, &dir);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_lsdir close error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_file_find(lfs_t* lfs, const char* path) {

  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, path, &info);
  if (lfs_err != LFS_ERR_OK) {
    // printf("grid_littlefs_file_find stat error: %d\n", lfs_err);
    return 1;
  }

  return info.type == LFS_TYPE_REG ? 0 : 1;
}

size_t grid_littlefs_file_size(lfs_t* lfs, const char* path) {

  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, path, &info);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_size stat error: %d\n", lfs_err);
    return 0;
  }

  return info.size;
}

int grid_littlefs_file_read(lfs_t* lfs, const char* path, uint8_t* buffer, uint16_t size) {

  lfs_file_t file;
  int lfs_err = lfs_file_open(lfs, &file, path, LFS_O_RDONLY);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_read open error: %d\n", lfs_err);
    return 1;
  }

  lfs_ssize_t read = lfs_file_read(lfs, &file, buffer, size);
  if (read < 0) {
    printf("grid_littlefs_file_read read error: %ld\n", read);
    return 1;
  }

  lfs_err = lfs_file_close(lfs, &file);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_read close error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_file_write(lfs_t* lfs, const char* path, const uint8_t* buffer, uint16_t size) {

  lfs_file_t file;
  int lfs_err = lfs_file_open(lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_write open error: %d\n", lfs_err);
    return 1;
  }

  lfs_ssize_t wrote = lfs_file_write(lfs, &file, buffer, size);
  if (wrote < 0) {
    printf("grid_littlefs_file_write write error: %ld\n", wrote);
    return 1;
  }

  lfs_err = lfs_file_close(lfs, &file);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_write close error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

struct lfs_info FIRSTDIR = {0};

void* grid_littlefs_dir_first(lfs_t* lfs, const char* path) {

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, path);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_dir_first open error: %d\n", lfs_err);
    return NULL;
  }

  struct lfs_info* ret = NULL;
  while (lfs_dir_read(lfs, &dir, &FIRSTDIR) > 0) {

    // Ignore entry for current directory
    if (strcmp(FIRSTDIR.name, ".") == 0) {
      continue;
    }

    // Ignore entry for parent directory
    if (strcmp(FIRSTDIR.name, "..") == 0) {
      continue;
    }

    ret = &FIRSTDIR;
    break;
  }

  lfs_err = lfs_dir_close(lfs, &dir);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_dir_first close error: %d\n", lfs_err);
    return NULL;
  }

  return ret;
}

size_t grid_littlefs_get_total_bytes(struct lfs_config* cfg) { return cfg->block_size * cfg->block_count; }

size_t grid_littlefs_get_used_bytes(lfs_t* lfs, struct lfs_config* cfg) {

  size_t fs_size = cfg->block_size * lfs_fs_size(lfs);

  // lfs_fs_size may return a size larger than the actual filesystem size
  size_t total = grid_littlefs_get_total_bytes(cfg);
  return fs_size > total ? total : fs_size;
}

lfs_file_t* grid_littlefs_fopen(lfs_t* lfs, const char* path, const char* mode) {

  lfs_file_t* file = malloc(sizeof(lfs_file_t));
  if (!file) {
    printf("grid_littlefs_fopen malloc failed\n");
    return NULL;
  }

  int flags = 0;
  if (strcmp(mode, "r") == 0) {
    flags = LFS_O_RDONLY;
  }
  if (strcmp(mode, "w") == 0) {
    flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
  }
  if (strcmp(mode, "a") == 0) {
    flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
  }
  if (strcmp(mode, "r+") == 0) {
    flags = LFS_O_RDWR;
  }
  if (strcmp(mode, "w+") == 0) {
    flags = LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC;
  }
  if (strcmp(mode, "a+") == 0) {
    flags = LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND;
  }

  if (flags == 0) {
    printf("grid_littlefs_fopen mode error: unsupported mode string\n");
    free(file);
    return NULL;
  }

  int lfs_err = lfs_file_open(lfs, file, path, flags);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_fopen open error: %d\n", lfs_err);
    printf("path: (%s)\n", path);
    free(file);
    return NULL;
  }

  return file;
}

int grid_littlefs_fclose(lfs_t* lfs, lfs_file_t* stream) {

  int lfs_err = lfs_file_close(lfs, stream);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_fclose close error: %d\n", lfs_err);
    return EOF;
  }

  free(stream);

  return 0;
}

size_t grid_littlefs_fwrite(lfs_t* lfs, const void* ptr, size_t size, size_t nmemb, lfs_file_t* stream) {

  size_t i = 0;
  while (i < nmemb) {

    uint8_t* src = ((uint8_t*)ptr) + i * size;
    lfs_ssize_t wrote = lfs_file_write(lfs, stream, src, size);

    if (wrote < 0) {
      printf("grid_littlefs_fwrite write error, wrote: %ld\n", wrote);
      break;
    }

    if (size != (size_t)wrote) {
      break;
    }

    ++i;
  }

  return i;
}

size_t grid_littlefs_fread(lfs_t* lfs, void* ptr, size_t size, size_t nmemb, lfs_file_t* stream) {

  size_t i = 0;
  while (i < nmemb) {

    uint8_t* dest = ((uint8_t*)ptr) + i * size;
    lfs_ssize_t read = lfs_file_read(lfs, stream, dest, size);

    if (read < 0) {
      printf("grid_littlefs_fread read error, read: %ld\n", read);
      break;
    }

    if (size != (size_t)read) {
      break;
    }

    ++i;
  }

  return i;
}

ssize_t grid_littlefs_ftell(lfs_t* lfs, lfs_file_t* stream) {

  lfs_soff_t soff = lfs_file_tell(lfs, stream);
  return soff >= 0 ? soff : -1;
}

ssize_t grid_littlefs_fseek(lfs_t* lfs, lfs_file_t* stream, lfs_soff_t soff, int whence) { return lfs_file_seek(lfs, stream, soff, whence) >= 0 ? 0 : -1; }

int grid_littlefs_fflush(lfs_t* lfs, lfs_file_t* stream) { return lfs_file_sync(lfs, stream) >= 0 ? 0 : EOF; }

lfs_dir_t* grid_littlefs_opendir(lfs_t* lfs, const char* name) {

  lfs_dir_t* dirp = malloc(sizeof(lfs_dir_t));
  if (!dirp) {
    printf("grid_littlefs_opendir malloc failed\n");
    return NULL;
  }

  int lfs_err = lfs_dir_open(lfs, dirp, name);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_opendir open error: %d\n", lfs_err);
    free(dirp);
    return NULL;
  }

  return dirp;
}

int grid_littlefs_closedir(lfs_t* lfs, lfs_dir_t* dirp) {

  int lfs_err = lfs_dir_close(lfs, dirp);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_closedir close error: %d\n", lfs_err);
    return -1;
  }

  free(dirp);

  return 0;
}

struct lfs_info READDIR = {0};

void* grid_littlefs_readdir(lfs_t* lfs, lfs_dir_t* dirp) { return lfs_dir_read(lfs, dirp, &READDIR) > 0 ? &READDIR : NULL; }

const char* grid_littlefs_file_info_name(struct lfs_info* info) { return info->name; }

uint8_t grid_littlefs_file_info_type(struct lfs_info* info) { return info->type; }

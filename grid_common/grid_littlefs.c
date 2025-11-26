#include "grid_littlefs.h"

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

int grid_littlefs_mkdir_base(lfs_t* lfs, const char* path) {

  if (path[0] == '\0') {
    return 0;
  }

  // Attempt to stat the directory at the base path
  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, path, &info);
  if (lfs_err == LFS_ERR_OK) {
    printf("directory at base path already exists\n");
    return 0;
  }

  // Attempt to make a directory at the base path
  printf("creating directory at base path...\n");
  lfs_err = lfs_mkdir(lfs, path);
  if (lfs_err != LFS_ERR_OK) {
    printf("failed to make directory at base path\n");
    return 1;
  }

  return 0;
}

int grid_littlefs_path_build(const char* path, uint16_t out_size, char* out) {

  assert(path != out);

  const char* prefix = grid_platform_get_base_path();

  if (strlen(prefix) + strlen(path) + 1 > out_size) {
    return 1;
  }

  sprintf(out, "%s/%s", prefix, path);

  // printf("grid_littlefs_path_build: \"%s\"\n", out);

  return 0;
}

int grid_littlefs_remove(lfs_t* lfs, const char* path) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  int lfs_err = lfs_remove(lfs, fpath);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_remove error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_mkdir(lfs_t* lfs, const char* path) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  int lfs_err = lfs_mkdir(lfs, fpath);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_mkdir error: %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_rmdir(lfs_t* lfs, const char* path) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, fpath);
  if (lfs_err != LFS_ERR_OK) {
    // printf("grid_littlefs_rmdir open error: %d\n", lfs_err);
    return 1;
  }

  printf("grid_littlefs_rmdir: \"%s\"\n", fpath);

  struct lfs_info info;
  while (lfs_dir_read(lfs, &dir, &info) > 0) {

    // Ignore entry for current directory
    if (info.name[0] == '.' && info.name[1] == '\0') {
      continue;
    }

    // Ignore entry for parent directory
    if (info.name[0] == '.' && info.name[1] == '.' && info.name[2] == '\0') {
      continue;
    }

    char path2[LFS_NAME_MAX * 2 + 2] = {0};
    sprintf(path2, "%s/%s", path, info.name);

    char fpath2[LFS_NAME_MAX + 1] = {0};
    if (grid_littlefs_path_build(path2, LFS_NAME_MAX + 1, fpath2)) {
      continue;
    }

    int lfs_err = lfs_remove(lfs, fpath2);
    if (lfs_err != LFS_ERR_OK) {
      printf("grid_littlefs_rmdir remove entry error %d\n", lfs_err);
    }
  }

  lfs_err = lfs_dir_close(lfs, &dir);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_rmdir close error: %d\n", lfs_err);
    return 1;
  }

  lfs_err = lfs_remove(lfs, fpath);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_rmdir remove error %d\n", lfs_err);
    return 1;
  }

  return 0;
}

int grid_littlefs_lsdir(lfs_t* lfs, const char* path) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  printf("list directory: \"%s\"\n", fpath);

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, fpath);
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

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, fpath, &info);
  if (lfs_err != LFS_ERR_OK) {
    // printf("grid_littlefs_file_find stat error: %d\n", lfs_err);
    return 1;
  }

  return info.type == LFS_TYPE_REG ? 0 : 1;
}

size_t grid_littlefs_file_size(lfs_t* lfs, const char* path) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  struct lfs_info info;
  int lfs_err = lfs_stat(lfs, fpath, &info);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_file_size stat error: %d\n", lfs_err);
    return 0;
  }

  return info.size;
}

int grid_littlefs_file_read(lfs_t* lfs, const char* path, uint8_t* buffer, uint16_t size) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  lfs_file_t file;
  int lfs_err = lfs_file_open(lfs, &file, fpath, LFS_O_RDONLY);
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

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  lfs_file_t file;
  int lfs_err = lfs_file_open(lfs, &file, fpath, LFS_O_WRONLY | LFS_O_CREAT);
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

size_t grid_littlefs_get_total_bytes(struct lfs_config* cfg) { return cfg->block_size * cfg->block_count; }

size_t grid_littlefs_get_used_bytes(lfs_t* lfs, struct lfs_config* cfg) {

  size_t fs_size = cfg->block_size * lfs_fs_size(lfs);

  // lfs_fs_size may return a size larger than the actual filesystem size
  size_t total = grid_littlefs_get_total_bytes(cfg);
  return fs_size > total ? total : fs_size;
}

int grid_littlefs_find_least_of_larger(lfs_t* lfs, const char* path, int* last_num) {

  char fpath[LFS_NAME_MAX + 1] = {0};
  grid_littlefs_path_build(path, LFS_NAME_MAX + 1, fpath);

  lfs_dir_t dir;
  int lfs_err = lfs_dir_open(lfs, &dir, fpath);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_lsdir open error: %d\n", lfs_err);
    return 1;
  }

  int min = INT_MAX;

  struct lfs_info info;
  while (lfs_dir_read(lfs, &dir, &info) > 0) {

    // Attempt to parse the name as two hexadecimal digits
    int index;
    int matched = sscanf(info.name, "%02x", &index);
    if (matched != 1) {
      continue;
    }

    // Consider the index when it is larger than the last number
    if (index > *last_num) {

      // If the index is smaller than the previous smallest, store it
      if (index < min) {
        min = index;
      }
    }
  }

  lfs_err = lfs_dir_close(lfs, &dir);
  if (lfs_err != LFS_ERR_OK) {
    printf("grid_littlefs_lsdir close error: %d\n", lfs_err);
    return 1;
  }

  int found = min != INT_MAX;

  *last_num = found ? min : -1;

  return found ? 0 : 1;
}

int grid_littlefs_find_next_on_page(lfs_t* lfs, uint8_t page, int* last_ele, int* last_evt) {

  char fpath[LFS_NAME_MAX + 1] = {0};

  while (true) {

    if (*last_evt < 0) {

      // Try to find next valid element
      sprintf(fpath, "%02x", page);
      grid_littlefs_find_least_of_larger(lfs, fpath, last_ele);

      // If no element is found, the traversal is over
      if (*last_ele < 0) {
        return 1;
      }
    }

    // Try to find next valid event
    sprintf(fpath, "%02x/%02x", page, *last_ele);
    grid_littlefs_find_least_of_larger(lfs, fpath, last_evt);

    // If a valid event is found, return success
    if (*last_evt >= 0) {
      return 0;
    }
  }
}

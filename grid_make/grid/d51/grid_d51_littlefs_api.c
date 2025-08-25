#include "grid_d51_module.h"

#include "grid_d51_littlefs_api.h"

int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {

  struct d51_littlefs_t* dfs = c->context;

  size_t offset = (block * c->block_size) + off;

  int err;

  CRITICAL_SECTION_ENTER();
  err = flash_read(dfs->flash, GRID_D51_NVM_BASE_ADDR + offset, buffer, size);
  CRITICAL_SECTION_LEAVE();
  if (err) {
    printf("failed to read addr %08x, size %08x, err %d\n", offset, size, err);
    return err;
  }

  return 0;
}

int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {

  struct d51_littlefs_t* dfs = c->context;

  size_t offset = (block * c->block_size) + off;

  int err;

  CRITICAL_SECTION_ENTER();
  err = flash_write(dfs->flash, GRID_D51_NVM_BASE_ADDR + offset, buffer, size);
  CRITICAL_SECTION_LEAVE();
  if (err) {
    printf("failed to write addr %08x, size %08x, err %d\n", offset, size, err);
    return err;
  }

  return 0;
}

int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block) {

  struct d51_littlefs_t* dfs = c->context;

  size_t offset = block * c->block_size;

  int err;

  size_t pages_in_block = GRID_D51_LITTLEFS_BLOCK_SIZE / GRID_D51_LITTLEFS_PAGE_SIZE;

  CRITICAL_SECTION_ENTER();
  err = flash_erase(dfs->flash, GRID_D51_NVM_BASE_ADDR + offset, pages_in_block);
  CRITICAL_SECTION_LEAVE();
  if (err) {
    printf("failed to erase addr %08x, pages %08x, err %d\n", offset, pages_in_block, err);
    return err;
  }

  return 0;
}

int littlefs_api_sync(const struct lfs_config* c) { return 0; }

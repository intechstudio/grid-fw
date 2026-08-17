#include "grid_rp2350_littlefs_api.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h" // XIP_BASE
#include "hardware/sync.h"

int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {

  size_t offset = (block * c->block_size) + off;

  // Read straight from the memory-mapped XIP window.
  memcpy(buffer, (const void*)(XIP_BASE + GRID_RP2350_FS_BASE + offset), size);

  return 0;
}

int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {

  size_t offset = (block * c->block_size) + off;

  // XIP is disabled while flash is programmed; mask interrupts so no
  // flash-resident code (e.g. the ADC DMA ISR) executes meanwhile.
  // NOTE: single-core only. If core1 is ever used, replace this with
  // flash_safe_execute() so the other core is locked out too.
  uint32_t ints = save_and_disable_interrupts();
  flash_range_program(GRID_RP2350_FS_BASE + offset, (const uint8_t*)buffer, size);
  restore_interrupts(ints);

  return 0;
}

int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block) {

  size_t offset = block * c->block_size;

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(GRID_RP2350_FS_BASE + offset, GRID_RP2350_FS_BLOCK_SIZE);
  restore_interrupts(ints);

  return 0;
}

int littlefs_api_sync(const struct lfs_config* c) { return 0; }

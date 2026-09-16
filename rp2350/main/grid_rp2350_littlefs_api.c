#include "grid_rp2350_littlefs_api.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/irq.h"             // PICO_HIGHEST_IRQ_PRIORITY, PICO_DEFAULT_IRQ_PRIORITY
#include "hardware/regs/addressmap.h" // XIP_BASE
#include "hardware/sync.h"

// BASEPRI masks everything but the boosted UART RX IRQ (DMA_IRQ_2) during
// flash ops, so daisy-chain traffic isn't dropped. Single-core only -- needs flash_safe_execute() for core1.
enum { GRID_RP2350_FLASH_SAFE_BASEPRI = PICO_DEFAULT_IRQ_PRIORITY / 2 };

// Raw BASEPRI mrs/msr, mirroring PRIMASK access in hardware/sync.h -- no
// pico-sdk BASEPRI helper exists.
static inline uint32_t grid_rp2350_flash_safe_mask_begin(void) {
  uint32_t old_basepri;
  __asm volatile("mrs %0, basepri" : "=r"(old_basepri));
  __asm volatile("msr basepri, %0" ::"r"((uint32_t)GRID_RP2350_FLASH_SAFE_BASEPRI) : "memory");
  return old_basepri;
}

static inline void grid_rp2350_flash_safe_mask_end(uint32_t old_basepri) { __asm volatile("msr basepri, %0" ::"r"(old_basepri) : "memory"); }

int littlefs_api_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {

  size_t offset = (block * c->block_size) + off;

  // Read straight from the memory-mapped XIP window.
  memcpy(buffer, (const void*)(XIP_BASE + GRID_RP2350_FS_BASE + offset), size);

  return 0;
}

int littlefs_api_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {

  size_t offset = (block * c->block_size) + off;

  uint32_t old_basepri = grid_rp2350_flash_safe_mask_begin();
  flash_range_program(GRID_RP2350_FS_BASE + offset, (const uint8_t*)buffer, size);
  grid_rp2350_flash_safe_mask_end(old_basepri);

  return 0;
}

int littlefs_api_erase(const struct lfs_config* c, lfs_block_t block) {

  size_t offset = block * c->block_size;

  uint32_t old_basepri = grid_rp2350_flash_safe_mask_begin();
  flash_range_erase(GRID_RP2350_FS_BASE + offset, GRID_RP2350_FS_BLOCK_SIZE);
  grid_rp2350_flash_safe_mask_end(old_basepri);

  return 0;
}

int littlefs_api_sync(const struct lfs_config* c) { return 0; }

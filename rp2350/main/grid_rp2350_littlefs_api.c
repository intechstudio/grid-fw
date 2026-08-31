#include "grid_rp2350_littlefs_api.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/irq.h"             // PICO_HIGHEST_IRQ_PRIORITY, PICO_DEFAULT_IRQ_PRIORITY
#include "hardware/regs/addressmap.h" // XIP_BASE
#include "hardware/sync.h"

// XIP is disabled while flash is programmed/erased, so no flash-resident code
// may execute meanwhile -- but a full save_and_disable_interrupts() (PRIMASK)
// would also stall the UART daisy-chain's RX completion IRQ for the entire
// operation (a sector erase routinely takes 100-400ms, versus ~10ms for a
// direction's RX buffer to fill at 2Mbaud), silently dropping traffic on any
// connected neighbor every time a config/cal save runs.
//
// grid_rp2350_uart_rx_dma_irq (DMA_IRQ_2) is RAM-resident and boosted to
// PICO_HIGHEST_IRQ_PRIORITY specifically so it can keep running here (see
// grid_rp2350_uart_init). Every other interrupt in this firmware -- ADC,
// encoder, USB, the ms tick timer -- sits at PICO_DEFAULT_IRQ_PRIORITY, the
// pico-sdk's own documented default for every interrupt that doesn't
// explicitly change it (hardware/irq.h), and none of them do. Raising
// BASEPRI to a threshold strictly between the two blocks all of those while
// leaving DMA_IRQ_2 free to preempt, without needing to enumerate those
// other sources by name (and risk missing one -- a miss here means a hard
// fault instead of a dropped byte, since XIP really is disabled for anything
// that isn't RAM-resident).
//
// NOTE: single-core only. If core1 is ever used, this additionally needs
// flash_safe_execute() to lock out the other core.
enum { GRID_RP2350_FLASH_SAFE_BASEPRI = PICO_DEFAULT_IRQ_PRIORITY / 2 };

// Raw mrs/msr on the BASEPRI special register, mirroring save_and_disable_
// interrupts()/restore_interrupts()'s own raw-asm PRIMASK access in
// hardware/sync.h -- there's no pico-sdk-level BASEPRI helper, and this
// avoids depending on the CMSIS __get/set_BASEPRI intrinsics being visible
// from whatever happens to already be included here.
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

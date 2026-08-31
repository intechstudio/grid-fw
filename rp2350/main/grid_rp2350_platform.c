#include "grid_rp2350_platform.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"

#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_rp2350_uart.h"
#include "grid_swsr.h"
#include "grid_ui.h"

// RP2040/RP2350 only expose a 64-bit flash unique ID (vs. the 128-bit IDs D51
// and ESP32 read), so the upper two words of the shared 4x uint32_t shape are
// left zeroed.
uint32_t grid_platform_get_id(uint32_t* return_array) {
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);

  memset(return_array, 0, 4 * sizeof(uint32_t));
  memcpy(return_array, board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);

  return 0;
}

uint64_t grid_platform_rtc_get_micros() { return time_us_64(); }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return time_us_64() - told; }

uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return t1 - t2; }

void grid_platform_delay_ms(uint32_t delay_milliseconds) { sleep_ms(delay_milliseconds); }

// HWCFG strap read: GPIO1=SHIFT, GPIO2=CLOCK, GPIO3=DATA, GPIO4=DATA2. Bit-bang
// protocol ported directly from d51n20a/grid/d51/grid_d51.c:521-564 (a
// 74HC165-style parallel-in-serial-out shift register, LSB first, 8 bits).
// GPIO1 is moved off stdio UART0 RX (see rp2350/main/CMakeLists.txt) to make
// room for SHIFT.
//
// Two board variants place the shift register on different DATA lines; only
// one is ever actually populated. Both DATA pins are pulled up internally so
// an unpopulated line floats high and reads back as 255 (all bits set) --
// whichever line reads something other than 255 is the real hwcfg value.
#define GRID_RP2350_HWCFG_SHIFT_PIN 1
#define GRID_RP2350_HWCFG_CLOCK_PIN 2
#define GRID_RP2350_HWCFG_DATA_PIN 3
#define GRID_RP2350_HWCFG_DATA2_PIN 4

uint32_t grid_platform_get_hwcfg() {

  gpio_init(GRID_RP2350_HWCFG_SHIFT_PIN);
  gpio_init(GRID_RP2350_HWCFG_CLOCK_PIN);
  gpio_init(GRID_RP2350_HWCFG_DATA_PIN);
  gpio_init(GRID_RP2350_HWCFG_DATA2_PIN);
  gpio_set_dir(GRID_RP2350_HWCFG_SHIFT_PIN, GPIO_OUT);
  gpio_set_dir(GRID_RP2350_HWCFG_CLOCK_PIN, GPIO_OUT);
  gpio_set_dir(GRID_RP2350_HWCFG_DATA_PIN, GPIO_IN);
  gpio_set_dir(GRID_RP2350_HWCFG_DATA2_PIN, GPIO_IN);
  gpio_pull_up(GRID_RP2350_HWCFG_DATA_PIN);
  gpio_pull_up(GRID_RP2350_HWCFG_DATA2_PIN);

  gpio_put(GRID_RP2350_HWCFG_SHIFT_PIN, 0);
  sleep_ms(1);
  gpio_put(GRID_RP2350_HWCFG_SHIFT_PIN, 1);
  sleep_ms(1);
  gpio_put(GRID_RP2350_HWCFG_SHIFT_PIN, 0);

  uint8_t hwcfg_value = 0;
  uint8_t hwcfg_value2 = 0;

  for (uint8_t i = 0; i < 8; i++) {

    gpio_put(GRID_RP2350_HWCFG_SHIFT_PIN, 1);
    sleep_ms(1);

    if (gpio_get(GRID_RP2350_HWCFG_DATA_PIN)) {
      hwcfg_value |= (1 << i);
    }
    if (gpio_get(GRID_RP2350_HWCFG_DATA2_PIN)) {
      hwcfg_value2 |= (1 << i);
    }

    if (i != 7) {

      gpio_put(GRID_RP2350_HWCFG_CLOCK_PIN, 1);
      sleep_ms(1);
      gpio_put(GRID_RP2350_HWCFG_CLOCK_PIN, 0);
    }
  }

  return hwcfg_value != 255 ? hwcfg_value : hwcfg_value2;
}

uint8_t grid_platform_get_random_8() { return (uint8_t)(get_rand_32() & 0xFF); }

uint8_t grid_platform_get_reset_cause() { return watchdog_caused_reboot() ? 1 : 0; }

void grid_platform_system_reset() { watchdog_reboot(0, 0, 0); }

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = malloc(size);
  if (handle == NULL) {

    printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void grid_platform_printf_nonprint(const uint8_t* src, size_t size) {

  for (size_t i = 0; i < size; ++i) {

    printf(src[i] < 32 ? "[%02hhx]" : "%c", src[i]);
  }
}

void grid_platform_lcd_set_backlight(uint8_t backlight) {}

uint8_t grid_platform_get_adc_bit_depth() { return 12; }

// RP2350's 4-way USART daisy-chain transport lives in grid_rp2350_uart.c
// (PIO+DMA per direction, mirroring D51's real-DMA model -- see that file's
// header comment for why PIO rather than a real UART peripheral). These 3
// hooks are the entire grid_platform_* surface it needs.
// grid_platform_enable/disable_grid_transmitter (present on D51) have no
// callers anywhere in the shared codebase -- dead code there, skipped here.
uint32_t grid_platform_get_frame_len(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  return grid_rp2350_uart_tx_busy(dir);
}

void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir) {

  assert(swsr);
  assert(size > 0);
  assert(size <= GRID_PARAMETER_SPI_TRANSACTION_length - 1);
  assert(dir < GRID_RP2350_UART_DIR_COUNT);
  assert(grid_swsr_readable(swsr, size));
  assert(!grid_rp2350_uart_tx_busy(dir));

  struct grid_swsr_t* tx = (struct grid_swsr_t*)swsr;

  grid_swsr_read(tx, grid_rp2350_uart_tx_buf[dir], size);

  grid_rp2350_uart_tx_start(dir, size);
}

// direction arrives in either representation depending on the caller:
// grid_port_softreset (grid_port.c) passes the raw enum grid_port_dir (0-3),
// while grid_rp2350_uart_port_recv passes the same raw enum too -- but D51's
// own grid_d51_port_recv_uwsr instead passes grid_port_dir_to_code's
// GRID_CONST_NORTH.. range (0x11-0x14), which its own implementation expects
// and grid_port_softreset's call does not actually match. Normalizing here
// accepts either range so this hook is correct regardless of which
// convention a given caller uses.
uint8_t grid_platform_reset_grid_transmitter(uint8_t direction) {

  uint8_t dir = direction >= GRID_CONST_NORTH ? direction - GRID_CONST_NORTH : direction;
  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  grid_rp2350_uart_port_reset_dma(dir);

  return 0;
}

// No touch element on BU16 -- both platform-provided element-state tables
// (required by grid_ui.c) stay empty, matching D51
// (d51n20a/grid/d51/grid_d51.c:707-709).
const grid_ui_element_state_any_t grid_ui_element_state_anys[GRID_PARAMETER_ELEMENT_COUNT] = {0};
const grid_ui_element_state_reset_t grid_ui_element_state_resets[GRID_PARAMETER_ELEMENT_COUNT] = {0};

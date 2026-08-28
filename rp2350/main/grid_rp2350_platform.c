#include "grid_rp2350_platform.h"

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

// RP2350 has no module-to-module USART transport (see grid_transport.c's
// 2-port UI+USB layout). These are only ever reached from
// grid_port_send_usart/grid_port_softreset (grid_port.c), which RP2350's
// transport never calls at runtime -- but grid_port.c is one translation
// unit, so the symbols still need to link.
uint32_t grid_platform_get_frame_len(uint8_t dir) { return 0; }

void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir) {}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction) { return 1; }

// No touch element on BU16 -- both platform-provided element-state tables
// (required by grid_ui.c) stay empty, matching D51
// (d51n20a/grid/d51/grid_d51.c:707-709).
const grid_ui_element_state_any_t grid_ui_element_state_anys[GRID_PARAMETER_ELEMENT_COUNT] = {0};
const grid_ui_element_state_reset_t grid_ui_element_state_resets[GRID_PARAMETER_ELEMENT_COUNT] = {0};

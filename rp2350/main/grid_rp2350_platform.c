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
#include "grid_usb.h"

void grid_platform_get_id(uint32_t id[4]) {

  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);

  memset(id, 0, sizeof(uint32_t[4]));
  memcpy(id, board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
}

GRID_IRAM_ATTR uint64_t grid_platform_rtc_get_micros() { return time_us_64(); }

GRID_IRAM_ATTR uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return time_us_64() - told; }

GRID_IRAM_ATTR uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return t1 - t2; }

void grid_platform_delay_ms(uint32_t delay_milliseconds) { sleep_ms(delay_milliseconds); }

// HWCFG strap read: GPIO1=SHIFT, GPIO2=CLOCK, GPIO3=DATA, GPIO4=DATA2. Bit-bang
// GPIO1 is tied to UART0 RX so that's disabled in rp2350/main/CMakeLists.txt
//
// Two board variants place the shift register on different DATA lines; only
// one is ever actually populated. Both DATA pins are pulled up internally so
// an unpopulated line floats high and reads back as 255 (all bits set) --
// whichever line reads something other than 255 is the real hwcfg value.
#define RP2350_PIN_HWCFG_SHIFT 1
#define RP2350_PIN_HWCFG_CLOCK 2
#define RP2350_PIN_HWCFG_DATA 3
#define RP2350_PIN_HWCFG_DATA2 4

uint32_t grid_platform_get_hwcfg() {

  gpio_init(RP2350_PIN_HWCFG_SHIFT);
  gpio_set_dir(RP2350_PIN_HWCFG_SHIFT, GPIO_OUT);

  gpio_init(RP2350_PIN_HWCFG_CLOCK);
  gpio_set_dir(RP2350_PIN_HWCFG_CLOCK, GPIO_OUT);

  gpio_init(RP2350_PIN_HWCFG_DATA);
  gpio_set_dir(RP2350_PIN_HWCFG_DATA, GPIO_IN);
  gpio_pull_up(RP2350_PIN_HWCFG_DATA);

  gpio_init(RP2350_PIN_HWCFG_DATA2);
  gpio_set_dir(RP2350_PIN_HWCFG_DATA2, GPIO_IN);
  gpio_pull_up(RP2350_PIN_HWCFG_DATA2);

  gpio_put(RP2350_PIN_HWCFG_SHIFT, 0);
  gpio_put(RP2350_PIN_HWCFG_CLOCK, 1);

  sleep_us(40);

  gpio_put(RP2350_PIN_HWCFG_SHIFT, 1);

  sleep_us(10);

  uint8_t hwcfg_value = 0;
  uint8_t hwcfg_value2 = 0;

  for (uint8_t i = 0; i < 8; i++) {

    gpio_put(RP2350_PIN_HWCFG_CLOCK, 0);

    if (gpio_get(RP2350_PIN_HWCFG_DATA)) {
      hwcfg_value |= (1 << i);
    }
    if (gpio_get(RP2350_PIN_HWCFG_DATA2)) {
      hwcfg_value2 |= (1 << i);
    }

    sleep_us(10);
    gpio_put(RP2350_PIN_HWCFG_CLOCK, 1);
    sleep_us(10);
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

void grid_platform_sync1_pulse_send() {}

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

uint8_t grid_platform_stop_grid_transmitter(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  grid_rp2350_uart_port_stop_dma(dir);

  return 0;
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  grid_rp2350_uart_port_reset_dma(dir);

  return 0;
}

const grid_ui_element_state_any_t grid_ui_element_state_anys[GRID_PARAMETER_ELEMENT_COUNT] = {0};

const grid_ui_element_state_reset_t grid_ui_element_state_resets[GRID_PARAMETER_ELEMENT_COUNT] = {0};

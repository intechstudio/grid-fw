#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "grid_swsr.h"

// Defined in sim_core.c - captures a port's outbound bytes for
// grid_sim_port_drain_tx(). Not part of grid_platform.h because it's a
// simulator-only observability hook, not a hardware capability.
extern void grid_sim_port_tx_capture(uint8_t port, const char* data, uint32_t len);

// grid_platform_* implementation for the wasm module simulator (both the
// headless entry point and the SDL/browser GUI entry point link this).
//
// The simulator has no real filesystem, USART/SPI hardware, or NVM - the
// module's per-page/per-element script storage and inter-module transport
// hooks below are therefore all no-ops that report "nothing stored" /
// "nothing to send". This is intentional: it makes every element fall back
// to its baked-in default Lua script (see the GRID_ACTIONSTRING_* macros in
// common/src/c/grid_ui_*.h), which is what a from-scratch module simulation
// wants. grid_sim_load_config() drives grid_config_parse() directly instead
// of going through this file layer.
//
// grid_platform_rtc_* is the one hook with real behavior: it reads a virtual
// clock that only advances inside grid_sim_tick(), so the simulation is
// deterministic regardless of host speed (Node.js or a browser tab).

void* grid_platform_fopen(const char* pathname, const char* mode) { return NULL; }

int grid_platform_fclose(void* stream) { return 0; }

size_t grid_platform_fwrite(const void* ptr, size_t size, size_t nmemb, void* stream) { return 0; }

size_t grid_platform_fread(void* ptr, size_t size, size_t nmemb, void* stream) { return 0; }

long grid_platform_ftell(void* stream) { return 0; }

int grid_platform_fseek(void* stream, long offset, int whence) { return 0; }

void grid_platform_clearerr(void* stream) {}

int grid_platform_ferror(void* stream) { return 0; }

int grid_platform_getc(void* stream) { return 0; }

int grid_platform_ungetc(int c, void* stream) { return 0; }

int grid_platform_fflush(void* stream) { return 0; }

int grid_platform_remove(const char* pathname) { return 0; }

int grid_platform_rename(const char* oldpath, const char* newpath) { return 0; }

void* grid_platform_opendir(const char* name) { return NULL; }

int grid_platform_closedir(void* dirp) { return 0; }

void* grid_platform_readdir(void* dirp) { return NULL; }

const char* grid_platform_file_info_name(void* info) { return NULL; }

uint8_t grid_platform_file_info_type(void* info) { return 0; }

uint32_t grid_platform_file_info_size(void* info) { return 0; }

int grid_platform_mkdir(const char* path) { return 0; }

int grid_platform_lsdir(const char* path) { return 0; }

int grid_platform_stat(const char* path, void** statbuf) { return -1; }

int grid_platform_find_file(const char* path, struct grid_file_t* handle) { return 0; }

void* grid_platform_dir_first(const char* path) { return NULL; }

char* grid_platform_read_file_contents(const char* path) { return NULL; }

int grid_platform_write_file_contents(const char* buf, const char* path) { return 0; }

void grid_platform_delete_script_files_all() {}

void grid_platform_nvm_defrag() {}

void grid_platform_nvm_erase() {}

void grid_platform_nvm_format_and_mount() {}

const char* grid_platform_get_base_path() { return "/"; }

uint8_t grid_platform_get_nvm_state() {
  // "Is storage mounted/usable" - grid_ui_bulk_page_load() (common/src/c/
  // grid_ui.c) PT_EXITs immediately otherwise, skipping page/element setup
  // entirely (including running init.lua, which wires up per-element LED
  // color helpers - see common/src/lua/init.lua, simplecolor.lua). This
  // simulator has no real storage, but that's fine: every individual file
  // read below still correctly reports "not found" (see grid_platform_stat
  // et al.), so the normal embedded-default fallback path runs, same as a
  // freshly formatted real device.
  return 1;
}

uint8_t grid_platform_erase_nvm_next() { return 0; }

void grid_platform_set_lfs(void* lfs) {}

void grid_platform_system_reset() {
  // No real reset target to jump to - a firmware reset just re-runs
  // grid_sim_init() from JS, so this is intentionally a no-op.
}

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void grid_platform_printf_nonprint(const uint8_t* src, size_t size) {

  for (size_t i = 0; i < size; ++i) {
    uint8_t c = src[i];
    if (c >= 0x20 && c < 0x7f) {
      putchar(c);
    } else {
      printf("\\x%02x", c);
    }
  }
}

void grid_platform_delay_ms(uint32_t delay_milliseconds) {}

uint32_t grid_platform_get_id(uint32_t* return_array) {

  // Fixed, deterministic simulated unique id.
  return_array[0] = 0x53494d31; // "SIM1"
  return_array[1] = 0x56534e31; // "VSN1"
  return_array[2] = 0;
  return_array[3] = 0;

  return 4;
}

uint32_t grid_platform_get_hwcfg() { return 0; }

uint32_t grid_platform_get_hwcfg_bit(uint8_t n) { return 0; }

static uint32_t sim_random_state = 0x9e3779b9u;

uint8_t grid_platform_get_random_8() {

  // Simple deterministic xorshift PRNG - good enough for simulated jitter,
  // and reproducible across runs, which is what a simulator wants.
  sim_random_state ^= sim_random_state << 13;
  sim_random_state ^= sim_random_state >> 17;
  sim_random_state ^= sim_random_state << 5;

  return (uint8_t)(sim_random_state >> 24);
}

uint8_t grid_platform_get_reset_cause() { return 0; }

// Virtual clock in microseconds. Only advances inside grid_sim_tick(), so
// the simulation is deterministic regardless of host wall-clock speed.
static uint64_t sim_clock_us = 0;

void grid_sim_platform_advance_clock(uint32_t dt_ms) { sim_clock_us += (uint64_t)dt_ms * 1000; }

uint64_t grid_platform_rtc_get_micros() { return sim_clock_us; }

uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return t1 >= t2 ? t1 - t2 : t2 - t1; }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_diff(sim_clock_us, told); }

uint32_t grid_platform_get_frame_len(uint8_t dir) { return 0; }

void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir) {
  // No real UART to send over, but grid_port_send_usart() (common/src/c/
  // grid_port.c) still expects this to drain the swsr it hands us - that's
  // how it knows the frame went out and its tx queue can advance. Capture
  // the bytes instead of just dropping them, so grid_sim_port_drain_tx()
  // can show what this module would have sent over that USART port.
  struct grid_swsr_t* tx = (struct grid_swsr_t*)swsr;

  char buf[2048];
  uint32_t n = size < sizeof(buf) ? size : (uint32_t)sizeof(buf);
  grid_swsr_read(tx, buf, (int)n);
  grid_sim_port_tx_capture(dir, buf, n);

  // The swsr must end up fully drained of this frame regardless of the
  // capture buffer's size, or the next grid_swsr_until_msg_end() call gets
  // confused by the leftover tail.
  for (uint32_t remaining = size - n; remaining > 0;) {
    char discard[256];
    uint32_t chunk = remaining < sizeof(discard) ? remaining : (uint32_t)sizeof(discard);
    grid_swsr_read(tx, discard, (int)chunk);
    remaining -= chunk;
  }
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction) { return 0; }

void* grid_platform_allocate_volatile(size_t size) {
  // grid_ui/grid_ain/grid_msg use this for their one-time model allocations.
  // A plain malloc is all a single simulated module needs.
  return malloc(size);
}

uint8_t grid_platform_get_adc_bit_depth() { return 12; }

void grid_platform_mux_init(uint8_t mux_positions_bm) {}

void grid_platform_mux_write(uint8_t index) {}

void grid_platform_lcd_set_backlight(uint8_t backlight) {}

static const char HEX_DIGITS[] = "0123456789abcdef";

void grid_platform_byte_to_hex(uint8_t byte, char out[2]) {
  out[0] = HEX_DIGITS[(byte >> 4) & 0xf];
  out[1] = HEX_DIGITS[byte & 0xf];
}

void grid_platform_id_to_hex(const uint8_t* id, uint8_t byte_count, char* out) {
  for (uint8_t i = 0; i < byte_count; ++i) {
    grid_platform_byte_to_hex(id[i], out + i * 2);
  }
  out[byte_count * 2] = '\0';
}

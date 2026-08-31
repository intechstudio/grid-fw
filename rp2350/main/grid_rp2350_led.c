#include "grid_rp2350_led.h"

#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "hardware/dma.h"
#include "hardware/uart.h"

// The D51 driver uses a fixed SPI instance (GRID_LED); we fix UART1 the same
// way, but the TX pin varies by board variant (see grid_rp2350_led.h) so it's
// passed into grid_rp2350_led_init instead of hardcoded. UART runs at 3x the
// WS2812 symbol rate (3 UART bits/symbol).
#define GRID_LED_UART uart1
#define GRID_LED_BAUD 2400000

#define GRID_RP2350_LED_BYTES_PER_LED 8 // 24 bits / 3 symbols-per-byte

struct grid_rp2350_led_model grid_rp2350_led_state;

// Lookup table: 6 WS2812 bits (byte-aligned: 2 UART bytes) -> encoded pair.
// Generated in init so per-frame rendering is just table reads, mirroring the
// D51 driver's grid_led_color_code[256] table. 64 * 2 = 128 bytes of RAM.
static uint8_t grid_led_color_code[64][2];

static int grid_led_dma_chan;

// Encode 3 WS2812 bits (d1 earliest in time) into one UART byte.
//
// Physical (post-invert) frame bits we want: start=H, data b0..b7, stop=L.
// Symbols laid over [start,b0,b1][b2,b3,b4][b5,b6,b7]:
//   physical data b0..b7 = d1,0,1, d2,0,1, d3,0
// The UART transmits the *logical* byte (LSB first); the GPIO override inverts
// every bit, so the logical byte is the bitwise-NOT of the physical pattern.
static inline uint8_t grid_led_encode3(uint8_t d1, uint8_t d2, uint8_t d3) {
  uint8_t phys = 0;
  phys |= (d1 ? 1u : 0u) << 0;
  phys |= 0u << 1;
  phys |= 1u << 2;
  phys |= (d2 ? 1u : 0u) << 3;
  phys |= 0u << 4;
  phys |= 1u << 5;
  phys |= (d3 ? 1u : 0u) << 6;
  phys |= 0u << 7;
  return (uint8_t)~phys;
}

void grid_rp2350_led_init(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod, uint8_t tx_pin, uint8_t tx_pin_func) {

  // Generate the lookup table for fast rendering.
  for (int v = 0; v < 64; v++) {
    // v = 6 bits b5..b0, b5 earliest in time. First byte carries b5,b4,b3.
    grid_led_color_code[v][0] = grid_led_encode3((v >> 5) & 1u, (v >> 4) & 1u, (v >> 3) & 1u);
    grid_led_color_code[v][1] = grid_led_encode3((v >> 2) & 1u, (v >> 1) & 1u, v & 1u);
  }

  rp_mod->led_count = led_mod->led_count;
  rp_mod->framebuffer_size = led_mod->led_count * GRID_RP2350_LED_BYTES_PER_LED;
  rp_mod->framebuffer = (uint8_t*)malloc(rp_mod->framebuffer_size * sizeof(uint8_t));

  // Encode rgb=(0,0,0) into every LED (0x00 bytes are not the "off" pattern).
  for (uint32_t i = 0; i < led_mod->led_count; i++) {
    grid_rp2350_led_set_color(rp_mod, i, 0, 0, 0);
  }

  // UART1 TX on tx_pin, inverted so idle/start/stop levels match WS2812.
  // tx_pin_func is the raw mux function-select number for this specific pin
  // (RP2350's expanded per-pin mux table means it isn't always the generic
  // GPIO_FUNC_UART value -- see grid_rp2350_led.h).
  uart_init(GRID_LED_UART, GRID_LED_BAUD);
  uart_set_format(GRID_LED_UART, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(GRID_LED_UART, true);
  gpio_set_function(tx_pin, tx_pin_func);
  gpio_set_outover(tx_pin, GPIO_OVERRIDE_INVERT);

  // DMA channel feeding the UART TX FIFO from the framebuffer.
  grid_led_dma_chan = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(grid_led_dma_chan);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, uart_get_dreq(GRID_LED_UART, true));
  dma_channel_configure(grid_led_dma_chan, &c, &uart_get_hw(GRID_LED_UART)->dr, rp_mod->framebuffer, rp_mod->framebuffer_size, false);
}

void grid_rp2350_led_set_color(struct grid_rp2350_led_model* mod, uint32_t led_index, uint8_t r, uint8_t g, uint8_t b) {

  uint32_t bits = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b; // GRB, MSB first
  uint8_t* out = &mod->framebuffer[led_index * GRID_RP2350_LED_BYTES_PER_LED];

  for (int i = 0; i < 4; i++) { // 4 groups of 6 bits -> 4 * 2 = 8 UART bytes
    uint8_t v = (bits >> (24 - (i + 1) * 6)) & 0x3Fu;
    out[i * 2 + 0] = grid_led_color_code[v][0];
    out[i * 2 + 1] = grid_led_color_code[v][1];
  }
}

void grid_rp2350_led_generate_frame(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod) {

  for (uint32_t i = 0; i < rp_mod->led_count; i++) {

    uint8_t gre = led_mod->led_frame_buffer[i * 3 + 0];
    uint8_t red = led_mod->led_frame_buffer[i * 3 + 1];
    uint8_t blu = led_mod->led_frame_buffer[i * 3 + 2];

    grid_rp2350_led_set_color(rp_mod, i, red, gre, blu);
  }
}

void grid_rp2350_led_start_transfer(struct grid_rp2350_led_model* rp_mod) {

  dma_channel_wait_for_finish_blocking(grid_led_dma_chan);
  dma_channel_transfer_from_buffer_now(grid_led_dma_chan, rp_mod->framebuffer, rp_mod->framebuffer_size);
}

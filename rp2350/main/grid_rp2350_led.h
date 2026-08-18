#ifndef GRID_RP2350_LED_H
#define GRID_RP2350_LED_H

#include <stdint.h>

#include "grid_led.h"

// WS2812 LED driver for RP2350, structured after the D51 SPI LED driver
// (d51n20a/grid/d51/grid_d51_led.c). The hardware SPI can't reach the LED data
// pin (GPIO4 = UART1 TX), so the protocol is clocked out of a UART TX pin via
// DMA instead: UART runs at 2.4 Mbaud so 3 UART bits == one WS2812 symbol, and
// one LED occupies 8 framebuffer bytes.

struct grid_rp2350_led_model {

  uint32_t led_count;
  uint32_t framebuffer_size;
  uint8_t* framebuffer;
};

extern struct grid_rp2350_led_model grid_rp2350_led_state;

void grid_rp2350_led_init(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod);

void grid_rp2350_led_generate_frame(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod);
void grid_rp2350_led_start_transfer(struct grid_rp2350_led_model* rp_mod);

void grid_rp2350_led_set_color(struct grid_rp2350_led_model* mod, uint32_t led_index, uint8_t r, uint8_t g, uint8_t b);

#endif /* GRID_RP2350_LED_H */

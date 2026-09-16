#ifndef GRID_RP2350_LED_H
#define GRID_RP2350_LED_H

#include <stdint.h>

#include "grid_led.h"

// WS2812 LED driver for RP2350, structured after the D51 SPI LED driver
// (d51n20a/grid/d51/grid_d51_led.c). The hardware SPI can't reach the LED data
// pin, so the protocol is clocked out of a UART1 TX pin via DMA instead: UART
// runs at 2.4 Mbaud so 3 UART bits == one WS2812 symbol, and one LED occupies
// 8 framebuffer bytes.
//
// tx_pin/tx_pin_func must select a valid UART1 TX alternate-function mux
// entry: GPIO6 function F11 on RP2350 board variants. BU16's current
// prototype hardware is a temporary exception, using GPIO4 function
// F2/GPIO_FUNC_UART instead (GPIO4 is SPI0 MISO on EF44's encoder chain, see
// grid_rp2350_encoder.h, so the two can't share a pin) -- once the newer
// fixed BU16 prototype lands it moves to GPIO6/F11 too. RP2350's per-pin mux
// table can reach the same peripheral via different function numbers on
// different pins, unlike RP2040's fixed function-2-is-always-UART layout, so
// the function number can't be hardcoded here regardless.

struct grid_rp2350_led_model {

  uint32_t led_count;
  uint32_t framebuffer_size;
  uint8_t* framebuffer;
};

extern struct grid_rp2350_led_model grid_rp2350_led_state;

void grid_rp2350_led_init(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod, uint8_t tx_pin, uint8_t tx_pin_func);

void grid_rp2350_led_generate_frame(struct grid_rp2350_led_model* rp_mod, struct grid_led_model* led_mod);
void grid_rp2350_led_start_transfer(struct grid_rp2350_led_model* rp_mod);

void grid_rp2350_led_set_color(struct grid_rp2350_led_model* mod, uint32_t led_index, uint8_t r, uint8_t g, uint8_t b);

#endif /* GRID_RP2350_LED_H */

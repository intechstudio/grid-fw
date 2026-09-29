#ifndef GRID_RP2350_LED_H
#define GRID_RP2350_LED_H

#include <stdint.h>

#include "grid_led.h"

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

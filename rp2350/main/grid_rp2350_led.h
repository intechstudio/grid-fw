#ifndef GRID_RP2350_LED_H
#define GRID_RP2350_LED_H

#include <stdint.h>

#include "grid_led.h"

struct grid_rp2350_led_model {

  uint32_t led_count;
  uint32_t framebuffer_size;
  uint8_t* framebuffer;
  uint8_t divider;
};

extern struct grid_rp2350_led_model grid_rp2350_led_state;

void grid_rp2350_led_init(struct grid_rp2350_led_model* led, uint8_t divider);

void grid_rp2350_led_generate_frame(struct grid_rp2350_led_model* led);

void grid_rp2350_led_start_transfer(struct grid_rp2350_led_model* led);

#endif /* GRID_RP2350_LED_H */

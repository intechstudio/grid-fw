#ifndef GRID_RP2350_TOUCH_H
#define GRID_RP2350_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "grid_maxtouch.h"

struct grid_rp2350_touch_model {

  uint rst;
  uint chg;

  struct mxt_data data;
};

extern struct grid_rp2350_touch_model grid_rp2350_touch_state;

bool grid_rp2350_touch_init(struct grid_rp2350_touch_model* touch, uint scl_gpio, uint sda_gpio, uint reset_gpio, uint int_gpio, uint32_t i2c_freq_hz);

#endif /* GRID_RP2350_TOUCH_H */

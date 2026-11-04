#ifndef GRID_RP2350_TOUCH_H
#define GRID_RP2350_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "grid_maxtouch.h"
#include "grid_platform.h"

enum { MXT_I2C_ADDR_BYTES = 3 };

struct grid_rp2350_touch_model {

  uint rst;
  uint chg;

  i2c_inst_t* i2c_inst;
  uint8_t dev_addr;
  uint irq_num;

  uint16_t tx_length;
  uint16_t* tx_buffer;

  grid_maxtouch_chg_cb_t chg_cb;
  void* chg_cb_user;

  grid_maxtouch_async_cb_t async_cb;
  void* async_cb_user;
};

extern struct grid_rp2350_touch_model grid_rp2350_touch_state;

void grid_rp2350_touch_poll(struct grid_rp2350_touch_model* touch);

bool grid_rp2350_touch_init(struct grid_rp2350_touch_model* touch, uint scl_gpio, uint sda_gpio, uint reset_gpio, uint int_gpio, uint32_t i2c_freq_hz, struct mxt_data* data);

bool grid_rp2350_touch_init_chg_cb(struct grid_rp2350_touch_model* touch, uint int_gpio, grid_maxtouch_chg_cb_t cb, void* user);

bool grid_rp2350_touch_init_async_cb(struct grid_rp2350_touch_model* touch, grid_maxtouch_async_cb_t cb, void* user);

#endif /* GRID_RP2350_TOUCH_H */

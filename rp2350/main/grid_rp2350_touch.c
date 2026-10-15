#include "grid_rp2350_touch.h"

#include <stdlib.h>
#include <string.h>

#include "grid_platform.h"

struct grid_rp2350_touch_model grid_rp2350_touch_state;

int grid_platform_mxt_read(uint16_t addr, uint16_t size, uint8_t* dest) {

  uint8_t tx[2] = {addr & 0xff, addr >> 8};

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  int ret = i2c_write_blocking(touch->instance, touch->dev_addr, tx, 2, false);

  if (ret < 0) {
    return ret;
  }

  ret = i2c_read_blocking(touch->instance, touch->dev_addr, dest, size, false);

  return ret < 0 ? ret : 0;
}

int grid_platform_mxt_write(uint16_t addr, uint16_t size, uint8_t* src) {

  uint8_t* buf = grid_platform_allocate_volatile(size + 2);
  buf[0] = addr & 0xff;
  buf[1] = (addr >> 8) & 0xff;
  memcpy(&buf[2], src, size);

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  int ret = i2c_write_blocking(touch->instance, touch->dev_addr, buf, size + 2, false);

  free(buf);

  return ret < 0 ? ret : 0;
}

static void grid_rp2350_touch_init_gpio(struct grid_rp2350_touch_model* touch, uint reset, uint change) {

  touch->rst = reset;

  gpio_init(touch->rst);
  gpio_set_dir(touch->rst, GPIO_OUT);

  touch->chg = change;

  gpio_init(touch->chg);
  gpio_set_dir(touch->chg, GPIO_IN);
  gpio_pull_up(touch->chg);
}

static bool grid_rp2350_touch_reset(struct grid_rp2350_touch_model* touch) {

  // Hold RST low for at least 90 ns
  gpio_put(touch->rst, 0);
  sleep_ms(20);

  // Send RST high for more than 39 ms (typical hardware reset to CHG low time)
  gpio_put(touch->rst, 1);
  sleep_ms(100);

  // CHG should be low at this point
  return gpio_get(touch->chg) == 0;
}

static bool grid_rp2350_touch_init_i2c(struct grid_rp2350_touch_model* touch, uint sda, uint scl, uint32_t scl_freq) {

  touch->instance = i2c1;
  touch->dev_addr = 0x4A;

  i2c_init(touch->instance, scl_freq);

  gpio_set_function(sda, GPIO_FUNC_I2C);
  gpio_set_function(scl, GPIO_FUNC_I2C);

  // Disable internal pulls, external pull-ups are present
  gpio_disable_pulls(sda);
  gpio_disable_pulls(scl);
}

bool CHG_EDGE_FALL = false;

static void GRID_IRAM_ATTR mxt_chg_isr(uint gpio, uint32_t event_mask) { CHG_EDGE_FALL = true; }

void grid_rp2350_touch_poll(struct grid_rp2350_touch_model* touch) {

  if (!CHG_EDGE_FALL) {
    return;
  }

  grid_maxtouch_process_msgs(&touch->data);

  CHG_EDGE_FALL = false;
}

bool grid_rp2350_touch_init(struct grid_rp2350_touch_model* touch, uint scl_gpio, uint sda_gpio, uint reset_gpio, uint int_gpio, uint32_t i2c_freq_hz) {

  // Initialize I2C configuration
  grid_rp2350_touch_init_i2c(touch, sda_gpio, scl_gpio, i2c_freq_hz);

  // Initialize RST and INT/CHG GPIOs
  grid_rp2350_touch_init_gpio(touch, reset_gpio, int_gpio);

  // Perform reset sequence
  if (!grid_rp2350_touch_reset(touch)) {

    // Ignore return status, as success cannot be reliably detected due to some
    // bootloader versions not asserting the CHG line after bootloading
    printf("grid_rp2350_touch_init: CHG was high after reset\n");
  }

  // Read information block
  if (!grid_maxtouch_read_info_block(&touch->data)) {
    return false;
  }

  // Initialize debug diagnostics
  grid_maxtouch_debug_init(&touch->data);

  // Allocate message buffer
  touch->data.msg_buf = grid_platform_allocate_volatile(touch->data.T5_msg_size * touch->data.max_reportid);
  assert(touch->data.msg_buf);

  // Set up per-pin interrupt for CHG
  gpio_set_irq_enabled_with_callback(int_gpio, GPIO_IRQ_EDGE_FALL, true, mxt_chg_isr);

  // Start communications with the device
  grid_maxtouch_process_msgs(&touch->data);

  return true;
}

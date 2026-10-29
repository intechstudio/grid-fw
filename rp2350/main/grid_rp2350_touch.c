#include "grid_rp2350_touch.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/dma.h"

struct grid_rp2350_touch_model grid_rp2350_touch_state;

static int grid_rp2350_touch_dma_tx_chan;
static int grid_rp2350_touch_dma_rx_chan;

static void grid_rp2350_touch_arm_dma_rx(struct grid_rp2350_touch_model* touch, uint16_t size, uint8_t* dest) {

  dma_channel_set_write_addr(grid_rp2350_touch_dma_rx_chan, dest, false);
  dma_channel_set_trans_count(grid_rp2350_touch_dma_rx_chan, size, false);

  dma_channel_set_read_addr(grid_rp2350_touch_dma_tx_chan, touch->tx_buffer, false);
  dma_channel_set_trans_count(grid_rp2350_touch_dma_tx_chan, 2 + size, false);

  dma_start_channel_mask((1u << grid_rp2350_touch_dma_tx_chan) | (1u << grid_rp2350_touch_dma_rx_chan));
}

static int grid_rp2350_touch_realloc_i2c(struct grid_rp2350_touch_model* touch, uint16_t rx_length) {

  assert(touch->tx_buffer);

  free(touch->tx_buffer);

  touch->tx_length = rx_length + 2;

  touch->tx_buffer = grid_platform_allocate_volatile(touch->tx_length * 2);
  if (!touch->tx_buffer) {
    return -ENOMEM;
  }

  return 0;
}

int grid_platform_mxt_read_async(uint16_t addr, uint16_t size, uint8_t* dest) {

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  int ret = 0;

  // Reallocate the command buffer if it is too small
  if (size > touch->tx_length - 2) {
    if ((ret = grid_rp2350_touch_realloc_i2c(touch, size))) {
      return ret;
    }
  }

  // Setup commands for each byte to write to the bus
  touch->tx_buffer[0] = addr & 0xff;
  touch->tx_buffer[1] = addr >> 8;

  // A restart should be generated before the first byte to be written
  touch->tx_buffer[0] |= I2C_IC_DATA_CMD_RESTART_BITS;

  // Set CMD bit to indicate bytes to be read by this master-receiver
  for (uint16_t i = 0; i < size; ++i) {
    touch->tx_buffer[2 + i] = I2C_IC_DATA_CMD_CMD_BITS;
  }

  // A restart should be generated before the first byte to be read
  touch->tx_buffer[2] |= I2C_IC_DATA_CMD_RESTART_BITS;

  // A stop should be generated after the last byte is read
  touch->tx_buffer[2 + size - 1] |= I2C_IC_DATA_CMD_STOP_BITS;

  grid_rp2350_touch_arm_dma_rx(&grid_rp2350_touch_state, size, dest);

  return ret;
}

GRID_IRAM_ATTR static void grid_rp2350_touch_dma_rx_irq(void) {

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  uint32_t status = i2c_get_hw(touch->i2c_inst)->intr_stat;

  bool success = true;

  if (status & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {

    i2c_get_hw(touch->i2c_inst)->clr_tx_abrt;

    success = false;
  }

  if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {

    i2c_get_hw(touch->i2c_inst)->clr_stop_det;

    assert(touch->async_cb);
    touch->async_cb(success, touch->async_cb_user);
  }

  if (status) {

    // Clear channel interrupt by writing a bitmask
    dma_hw->ints1 = 1u << grid_rp2350_touch_dma_rx_chan;
  }
}

int grid_platform_mxt_read(uint16_t addr, uint16_t size, uint8_t* dest) {

  uint8_t tx[2] = {addr & 0xff, addr >> 8};

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  int ret = i2c_write_blocking(touch->i2c_inst, touch->dev_addr, tx, 2, false);

  if (ret < 0) {
    return ret;
  }

  ret = i2c_read_blocking(touch->i2c_inst, touch->dev_addr, dest, size, false);

  return ret < 0 ? ret : 0;
}

int grid_platform_mxt_write(uint16_t addr, uint16_t size, uint8_t* src) {

  uint8_t* buf = grid_platform_allocate_volatile(size + 2);
  buf[0] = addr & 0xff;
  buf[1] = (addr >> 8) & 0xff;
  memcpy(&buf[2], src, size);

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  int ret = i2c_write_blocking(touch->i2c_inst, touch->dev_addr, buf, size + 2, false);

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

  touch->i2c_inst = i2c1;
  touch->dev_addr = 0x4A;
  touch->irq_num = DMA_IRQ_1;

  i2c_init(touch->i2c_inst, scl_freq);

  i2c_get_hw(touch->i2c_inst)->enable = 0;
  i2c_get_hw(touch->i2c_inst)->tar = touch->dev_addr;
  i2c_get_hw(touch->i2c_inst)->enable = 1;

  i2c_get_hw(touch->i2c_inst)->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

  gpio_set_function(sda, GPIO_FUNC_I2C);
  gpio_set_function(scl, GPIO_FUNC_I2C);

  // Disable internal pulls, external pull-ups are present
  gpio_disable_pulls(sda);
  gpio_disable_pulls(scl);

  // Allocate minimum length buffers to read the info struct at address zero
  touch->tx_length = sizeof(struct mxt_info) + 2;
  touch->tx_buffer = grid_platform_allocate_volatile(touch->tx_length * 2);

  grid_rp2350_touch_dma_tx_chan = dma_claim_unused_channel(true);
  dma_channel_config tx_cfg = dma_channel_get_default_config(grid_rp2350_touch_dma_tx_chan);
  channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&tx_cfg, true);
  channel_config_set_write_increment(&tx_cfg, false);
  channel_config_set_dreq(&tx_cfg, i2c_get_dreq(touch->i2c_inst, true));
  dma_channel_configure(grid_rp2350_touch_dma_tx_chan, &tx_cfg, &i2c_get_hw(touch->i2c_inst)->data_cmd, NULL, 0, false);

  grid_rp2350_touch_dma_rx_chan = dma_claim_unused_channel(true);
  dma_channel_config rx_cfg = dma_channel_get_default_config(grid_rp2350_touch_dma_rx_chan);
  channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&rx_cfg, false);
  channel_config_set_write_increment(&rx_cfg, true);
  channel_config_set_dreq(&rx_cfg, i2c_get_dreq(touch->i2c_inst, false));
  dma_channel_configure(grid_rp2350_touch_dma_rx_chan, &rx_cfg, NULL, &i2c_get_hw(touch->i2c_inst)->data_cmd, 0, false);

  dma_channel_set_irq1_enabled(grid_rp2350_touch_dma_rx_chan, true);
  irq_set_exclusive_handler(touch->irq_num, grid_rp2350_touch_dma_rx_irq);
  irq_set_enabled(touch->irq_num, true);
}

GRID_IRAM_ATTR static void grid_rp2350_touch_chg_irq(uint gpio, uint32_t event_mask) {

  struct grid_rp2350_touch_model* touch = &grid_rp2350_touch_state;

  assert(touch->chg_cb && touch->chg_cb_user);
  touch->chg_cb(touch->chg_cb_user);
}

bool grid_rp2350_touch_init(struct grid_rp2350_touch_model* touch, uint scl_gpio, uint sda_gpio, uint reset_gpio, uint int_gpio, uint32_t i2c_freq_hz, struct mxt_data* data) {

  touch->async_cb = NULL;

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
  if (!grid_maxtouch_read_info_block(data)) {
    return false;
  }

  // Initialize debug diagnostics
  grid_maxtouch_debug_init(data);

  // Allocate message buffer
  data->msg_buf = grid_platform_allocate_volatile(data->T5_msg_size * data->max_reportid);
  assert(data->msg_buf);

  return true;
}

bool grid_rp2350_touch_init_chg_cb(struct grid_rp2350_touch_model* touch, uint int_gpio, grid_maxtouch_chg_cb_t cb, void* user) {

  // Set callback
  touch->chg_cb = cb;

  // Set user pointer
  touch->chg_cb_user = user;

  // Initialize per-pin interrupt for CHG
  gpio_set_irq_enabled_with_callback(int_gpio, GPIO_IRQ_EDGE_FALL, true, grid_rp2350_touch_chg_irq);
}

bool grid_rp2350_touch_init_async_cb(struct grid_rp2350_touch_model* touch, grid_maxtouch_async_cb_t cb, void* user) {

  // Set callback
  touch->async_cb = cb;

  // Set user pointer
  touch->async_cb_user = user;
}

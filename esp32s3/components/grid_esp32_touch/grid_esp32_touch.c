/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_touch.h"

#include <errno.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#include "grid_math.h"
#include "grid_platform.h"

DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

i2c_master_dev_handle_t DEV_HNDL;

int grid_platform_mxt_read(uint16_t addr, uint16_t size, uint8_t* dest) {

  uint8_t tx[2] = {addr & 0xff, addr >> 8};

  return i2c_master_transmit_receive(DEV_HNDL, tx, 2, dest, size, -1);
}

int grid_platform_mxt_write(uint16_t addr, uint16_t size, uint8_t* src) {

  uint8_t* buf = grid_platform_allocate_volatile(size + 2);
  buf[0] = addr & 0xff;
  buf[1] = (addr >> 8) & 0xff;
  memcpy(&buf[2], src, size);

  esp_err_t err = i2c_master_transmit(DEV_HNDL, buf, size + 2, -1);

  free(buf);

  return err;
}

static void grid_esp32_touch_init_gpio(struct grid_esp32_touch_model* touch, gpio_num_t reset, gpio_num_t change) {

  touch->rst = reset;

  gpio_config_t rst_cfg = {
      .pin_bit_mask = 1ULL << touch->rst,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&rst_cfg));

  touch->chg = change;

  gpio_config_t chg_cfg = {
      .pin_bit_mask = 1ULL << touch->chg,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&chg_cfg));
}

static bool grid_esp32_touch_reset(struct grid_esp32_touch_model* touch) {

  // Hold RST low for at least 90 ns
  gpio_set_level(touch->rst, 0);
  vTaskDelay(pdMS_TO_TICKS(20));

  // Send RST high for more than 39 ms (typical hardware reset to CHG low time)
  gpio_set_level(touch->rst, 1);
  vTaskDelay(pdMS_TO_TICKS(100));

  // CHG should be low at this point
  return gpio_get_level(touch->chg) == 0;
}

static void grid_esp32_touch_init_i2c(struct grid_esp32_touch_model* touch, i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t scl_freq) {

  touch->bus_conf = (i2c_master_bus_config_t){
      .i2c_port = port,
      .sda_io_num = sda,
      .scl_io_num = scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&touch->bus_conf, &touch->bus_hndl));

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = 0x4A,
      .scl_speed_hz = scl_freq,
  };

  ESP_ERROR_CHECK(i2c_master_bus_add_device(touch->bus_hndl, &dev_cfg, &DEV_HNDL));
}

bool CHG_EDGE_FALL = false;

static void GRID_IRAM_ATTR mxt_chg_isr(void* user) { CHG_EDGE_FALL = true; }

void grid_esp32_touch_poll(struct grid_esp32_touch_model* touch) {

  if (!CHG_EDGE_FALL) {
    return;
  }

  grid_maxtouch_process_msgs(&touch->data);

  CHG_EDGE_FALL = false;
}

bool grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset, gpio_num_t change, uint32_t i2c_freq_hz) {

  // Initialize I2C configuration
  grid_esp32_touch_init_i2c(touch, i2c_port, sda_gpio, scl_gpio, i2c_freq_hz);

  // Initialize RST and INT/CHG GPIOs
  grid_esp32_touch_init_gpio(touch, reset, change);

  // Perform reset sequence
  if (!grid_esp32_touch_reset(touch)) {

    // Ignore return status, as success cannot be reliably detected due to some
    // bootloader versions not asserting the CHG line after bootloading
    ets_printf("grid_esp32_touch_init: CHG was high after reset\n");
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
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_set_intr_type(change, GPIO_INTR_NEGEDGE));
  ESP_ERROR_CHECK(gpio_isr_handler_add(change, mxt_chg_isr, NULL));
  ESP_ERROR_CHECK(gpio_intr_enable(change));

  // Start communications with the device
  grid_maxtouch_process_msgs(&touch->data);

  return true;
}

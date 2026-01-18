/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_usb_cdc.h"

#include "esp_log.h"
#include "rom/ets_sys.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"

#include "grid_transport.h"
#include "grid_protocol.h"

static const char* TAG = "USB_CDC";

struct grid_swsr_t cdc_rx;

static void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t* event) {

  size_t rx_size = 0;
  uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  esp_err_t err = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);

  if (err != ESP_OK) {
    return;
  }

  struct grid_swsr_t* rx = &cdc_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    grid_swsr_read(rx, NULL, grid_swsr_size(rx));
  }

  struct grid_msg msg;

  if (!grid_msg_from_swsr(&msg, rx)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) == 0) {
    grid_transport_recv_usb(&grid_transport_state, (uint8_t*)msg.data, msg.length);
  }
}

static uint8_t DRAM_ATTR usb_tx_ready = 0;

static void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t* event) {
  int dtr = event->line_state_changed_data.dtr;
  int rts = event->line_state_changed_data.rts;
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);

  usb_tx_ready = 1;
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
  tinyusb_cdcacm_write_flush(0, 0);
  usb_tx_ready = 1;
}

int32_t grid_platform_usb_serial_ready(void) {
  return usb_tx_ready;
}

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {

  esp_err_t status = 0;

  if (usb_tx_ready == 1) {

    usb_tx_ready = 0;

    uint32_t queued = tinyusb_cdcacm_write_queue(0, (const uint8_t*)buffer, length);

    if (queued != length) {
      ets_printf("CDC QUEUE ERROR: %d %d\r\n", queued, length);
      tinyusb_cdcacm_write_flush(0, 0);
    } else {
      status = tinyusb_cdcacm_write_flush(0, 0);
    }
  } else {

    status = tinyusb_cdcacm_write_flush(0, 0);

    if (status == ESP_OK) {
      usb_tx_ready = 1;
    }
  }

  return 1;
}

void grid_esp32_usb_cdc_init(void) {

  tinyusb_config_cdcacm_t acm_cfg = {
      .cdc_port = TINYUSB_CDC_ACM_0,
      .callback_rx = &tinyusb_cdc_rx_callback,
      .callback_rx_wanted_char = NULL,
      .callback_line_state_changed = NULL,
      .callback_line_coding_changed = NULL
  };

  ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));
  ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_0, CDC_EVENT_LINE_STATE_CHANGED, &tinyusb_cdc_line_state_changed_callback));

  // Allocate CDC RX buffer
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&cdc_rx, capacity) == 0);
}

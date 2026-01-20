/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_usb_cdc.h"

#include "esp_log.h"
#include "rom/ets_sys.h"
#include "tinyusb.h"

#include "grid_protocol.h"
#include "grid_transport.h"

#if CFG_TUD_CDC

static const char* TAG = "USB_CDC";

// CDC RX buffer size for reading from TinyUSB
#define CDC_RX_BUFSIZE 512

// RX buffer for accumulating incoming data
static struct grid_swsr_t cdc_rx;
static bool cdc_initialized = false;

// TX ready flag - set when we can transmit
static uint8_t DRAM_ATTR usb_tx_ready = 0;

// ========================= TinyUSB CDC Callbacks =============================== //

// Called when data is received from the host
void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  if (!cdc_initialized) {
    return;
  }

  uint8_t buf[CDC_RX_BUFSIZE];
  uint32_t rx_size = tud_cdc_read(buf, sizeof(buf));

  if (rx_size == 0) {
    return;
  }

  struct grid_swsr_t* rx = &cdc_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    // Buffer full, discard old data
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

// Called when transmission is complete
void tud_cdc_tx_complete_cb(uint8_t itf) {
  (void)itf;
  usb_tx_ready = 1;
}

// Called when line state changes (DTR/RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);

  // Ready to transmit when DTR is set (terminal connected)
  if (dtr) {
    usb_tx_ready = 1;
  }
}

// ========================= Platform API =============================== //

int32_t grid_platform_usb_serial_ready(void) { return usb_tx_ready; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {

  if (usb_tx_ready == 1) {
    usb_tx_ready = 0;

    uint32_t written = tud_cdc_write(buffer, length);

    if (written != length) {
      ets_printf("CDC WRITE ERROR: %ld %ld\r\n", written, length);
    }

    tud_cdc_write_flush();
  } else {
    // Try to flush any pending data
    tud_cdc_write_flush();
  }

  return 1;
}

void grid_esp32_usb_cdc_init(void) {
  // Allocate CDC RX buffer
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&cdc_rx, capacity) == 0);

  cdc_initialized = true;
  ESP_LOGI(TAG, "CDC initialized");
}

#else // !CFG_TUD_CDC - stub implementations

int32_t grid_platform_usb_serial_ready(void) { return 0; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {
  (void)buffer;
  (void)length;
  return 0;
}

void grid_esp32_usb_cdc_init(void) {}

#endif // CFG_TUD_CDC

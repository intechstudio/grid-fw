#include "grid_d51_usb_acm.h"

#include <assert.h>

#include "tusb.h"

#include "grid_msg.h"
#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_transport.h"
#include "grid_usb.h"

static struct grid_swsr_t s_cdc_rx;
static uint8_t s_usb_tx_ready = 0;

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  uint8_t buf[512];
  uint32_t rx_size = tud_cdc_read(buf, sizeof(buf));

  if (rx_size == 0) {
    return;
  }

  struct grid_swsr_t* rx = &s_cdc_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    grid_swsr_read(rx, NULL, grid_swsr_size(rx));
    grid_swsr_write(rx, buf, rx_size);
  }

  struct grid_msg msg;

  if (!grid_msg_from_swsr(&msg, rx)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) == 0) {
    grid_transport_recv_usb(&grid_transport_state, (uint8_t*)msg.data, msg.length);
  }
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
  (void)itf;
  s_usb_tx_ready = 1;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;
  if (dtr) {
    s_usb_tx_ready = 1;
  }
}

int32_t grid_platform_usb_serial_ready(void) { return s_usb_tx_ready; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {
  if (s_usb_tx_ready) {
    s_usb_tx_ready = 0;
    tud_cdc_write(buffer, length);
    tud_cdc_write_flush();
  } else {
    tud_cdc_write_flush();
  }
  return 1;
}

void grid_d51_usb_acm_init(void) {
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&s_cdc_rx, capacity) == 0);
}

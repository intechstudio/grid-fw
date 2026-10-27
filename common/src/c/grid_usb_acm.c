#include "grid_usb_acm.h"

#include <assert.h>

#include "tusb.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_transport.h"
#include "grid_usb.h"

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

  uint32_t available = tud_cdc_available();
  if (available == 0) {
    return;
  }

  struct grid_swsr_t* rx = &grid_usb_state.acm.rx;
  uint32_t to_read = available < sizeof(buf) ? available : sizeof(buf);

  if (!grid_swsr_writable(rx, to_read)) {
    return;
  }

  uint32_t rx_size = tud_cdc_read(buf, to_read);
  if (rx_size == 0) {
    return;
  }

  assert(rx_size <= to_read);
  assert(grid_swsr_writable(rx, rx_size));
  grid_swsr_write(rx, buf, rx_size);
}

void grid_usb_acm_rx_process(struct grid_usb_acm_model* acm) {

  struct grid_msg msg;
  if (!grid_msg_from_swsr(&msg, &acm->rx)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) == 0) {
    grid_transport_recv_usb(&grid_transport_state, (uint8_t*)msg.data, msg.length);
  }
}

void grid_usb_acm_rx_poll(struct grid_usb_acm_model* acm) {
  (void)acm;
  tud_cdc_rx_cb(0);
}

bool grid_usb_acm_dtr(struct grid_usb_acm_model* acm) {
  (void)acm;
  return tud_cdc_connected();
}

bool grid_usb_acm_tx_ready(struct grid_usb_acm_model* acm) {
  (void)acm;
  return tud_cdc_write_available() == CFG_TUD_CDC_TX_BUFSIZE;
}

bool grid_usb_acm_tx_busy(struct grid_usb_acm_model* acm) { return grid_usb_acm_dtr(acm) && !grid_usb_acm_tx_ready(acm); }

int32_t grid_usb_acm_write(struct grid_usb_acm_model* acm, char* buffer, uint32_t length) {
  if (!tud_cdc_connected()) {
    return 0;
  }

  if (tud_cdc_write_available() < length) {
    return 0;
  }

  uint32_t written = tud_cdc_write(buffer, length);
  tud_cdc_write_flush();
  return (int32_t)written;
}

void grid_usb_acm_init(struct grid_usb_acm_model* acm, uint16_t rx_buffer_size) { assert(grid_swsr_malloc(&acm->rx, rx_buffer_size) == 0); }

void grid_usb_acm_on_connect(struct grid_usb_acm_model* acm) { (void)acm; }

void grid_usb_acm_on_disconnect(struct grid_usb_acm_model* acm) { (void)acm; }

#include "grid_usb_acm.h"

#include <assert.h>

#include "tusb.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_transport.h"
#include "grid_usb.h"

#if CFG_TUD_CDC

struct grid_usb_acm_model grid_usb_acm_state;

static uint8_t acm_rx_buf[512];

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  uint32_t available = tud_cdc_available();
  if (available == 0) {
    return;
  }

  struct grid_swsr_t* rx = &grid_usb_acm_state.rx;
  uint32_t to_read = available < sizeof(acm_rx_buf) ? available : sizeof(acm_rx_buf);

  if (!grid_swsr_writable(rx, to_read)) {
    return;
  }

  uint32_t rx_size = tud_cdc_read(acm_rx_buf, to_read);
  if (rx_size == 0) {
    return;
  }

  assert(rx_size <= to_read);
  assert(grid_swsr_writable(rx, rx_size));
  grid_swsr_write(rx, acm_rx_buf, rx_size);
}

void grid_usb_acm_rx_process(struct grid_usb_acm_model* usb_acm) {

  struct grid_msg msg;
  if (!grid_msg_from_swsr(&msg, &usb_acm->rx)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) == 0) {
    grid_transport_recv_usb(&grid_transport_state, (uint8_t*)msg.data, msg.length);
  }
}

void grid_usb_acm_rx_poll(struct grid_usb_acm_model* usb_acm) {
  (void)usb_acm;
  tud_cdc_rx_cb(0);
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
  (void)itf;
  if (tud_cdc_write_available() == CFG_TUD_CDC_TX_BUFSIZE) {
    grid_usb_acm_state.tx_ready = 1;
  }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;
  grid_usb_acm_state.dtr = dtr;
  if (dtr) {
    grid_usb_acm_state.tx_ready = 1;
  }
}

int32_t grid_usb_acm_tx_ready(struct grid_usb_acm_model* usb_acm) { return usb_acm->tx_ready; }

bool grid_usb_acm_dtr(struct grid_usb_acm_model* usb_acm) { return usb_acm->dtr; }

int32_t grid_usb_acm_write(struct grid_usb_acm_model* usb_acm, char* buffer, uint32_t length) {
  if (!usb_acm->tx_ready || tud_cdc_write_available() < length) {
    usb_acm->tx_dropped++;
    return 0;
  }
  usb_acm->tx_ready = 0;
  uint32_t written = tud_cdc_write(buffer, length);
  if (written != length) {
    usb_acm->tx_dropped++;
  }
  tud_cdc_write_flush();
  return (int32_t)written;
}

void grid_usb_acm_init(struct grid_usb_acm_model* usb_acm, uint16_t rx_buffer_size) {
  assert(grid_swsr_malloc(&usb_acm->rx, rx_buffer_size) == 0);
  usb_acm->tx_ready = 1;
}

#else // !CFG_TUD_CDC

int32_t grid_usb_acm_tx_ready(struct grid_usb_acm_model* usb_acm) {
  (void)usb_acm;
  return 0;
}

bool grid_usb_acm_dtr(struct grid_usb_acm_model* usb_acm) {
  (void)usb_acm;
  return false;
}

int32_t grid_usb_acm_write(struct grid_usb_acm_model* usb_acm, char* buffer, uint32_t length) {
  (void)usb_acm;
  (void)buffer;
  (void)length;
  return 0;
}

void grid_usb_acm_init(struct grid_usb_acm_model* usb_acm, uint16_t rx_buffer_size) {
  (void)usb_acm;
  (void)rx_buffer_size;
}

void grid_usb_acm_rx_poll(struct grid_usb_acm_model* usb_acm) { (void)usb_acm; }

void grid_usb_acm_rx_process(struct grid_usb_acm_model* usb_acm) { (void)usb_acm; }

#endif // CFG_TUD_CDC

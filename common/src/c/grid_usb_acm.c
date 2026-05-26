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

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  if (!grid_usb_acm_state.initialized) {
    return;
  }

  static uint8_t buf[512];
  uint32_t rx_size = tud_cdc_read(buf, sizeof(buf));

  if (rx_size == 0) {
    return;
  }

  struct grid_swsr_t* rx = &grid_usb_acm_state.rx;

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
  grid_usb_acm_state.tx_ready = 1;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;
  if (dtr) {
    grid_usb_acm_state.tx_ready = 1;
  }
}

int32_t grid_usb_acm_ready(struct grid_usb_acm_model* usb_acm) { return usb_acm->tx_ready; }

int32_t grid_usb_acm_write(struct grid_usb_acm_model* usb_acm, char* buffer, uint32_t length) {
  if (usb_acm->tx_ready) {
    usb_acm->tx_ready = 0;
    uint32_t written = tud_cdc_write(buffer, length);
    if (written != length) {
      grid_platform_printf("CDC WRITE ERROR: %ld %ld\n", written, length);
    }
    tud_cdc_write_flush();
    return (int32_t)written;
  } else {
    tud_cdc_write_flush();
    return 0;
  }
}

void grid_usb_acm_init(struct grid_usb_acm_model* usb_acm, uint16_t rx_buffer_size) {
  assert(grid_swsr_malloc(&usb_acm->rx, rx_buffer_size) == 0);
  usb_acm->initialized = true;
  usb_acm->tx_ready = 1;
}

#else // !CFG_TUD_CDC

int32_t grid_usb_acm_ready(struct grid_usb_acm_model* usb_acm) {
  (void)usb_acm;
  return 0;
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

#endif // CFG_TUD_CDC

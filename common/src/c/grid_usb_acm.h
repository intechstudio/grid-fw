#ifndef GRID_USB_ACM_H
#define GRID_USB_ACM_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

struct grid_usb_acm_model {
  struct grid_swsr_t rx;
  uint32_t tx_dropped;
};

extern struct grid_usb_acm_model grid_usb_acm_state;

void grid_usb_acm_init(struct grid_usb_acm_model* usb_acm, uint16_t rx_buffer_size);
void grid_usb_acm_on_connect(struct grid_usb_acm_model* usb_acm);
void grid_usb_acm_on_disconnect(struct grid_usb_acm_model* usb_acm);

bool grid_usb_acm_dtr(struct grid_usb_acm_model* usb_acm);
bool grid_usb_acm_tx_ready(struct grid_usb_acm_model* usb_acm);
int32_t grid_usb_acm_write(struct grid_usb_acm_model* usb_acm, char* buffer, uint32_t length);
void grid_usb_acm_rx_poll(struct grid_usb_acm_model* usb_acm);
void grid_usb_acm_rx_process(struct grid_usb_acm_model* usb_acm);

#endif /* GRID_USB_ACM_H */

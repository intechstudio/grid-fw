#ifndef GRID_USB_ACM_H
#define GRID_USB_ACM_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

struct grid_usb_acm_model {
  struct grid_swsr_t rx;
};

void grid_usb_acm_init(struct grid_usb_acm_model* acm, uint16_t rx_buffer_size);
void grid_usb_acm_on_connect(struct grid_usb_acm_model* acm);
void grid_usb_acm_on_disconnect(struct grid_usb_acm_model* acm);

bool grid_usb_acm_dtr(struct grid_usb_acm_model* acm);
bool grid_usb_acm_tx_ready(struct grid_usb_acm_model* acm);
bool grid_usb_acm_tx_busy(struct grid_usb_acm_model* acm);
int32_t grid_usb_acm_write(struct grid_usb_acm_model* acm, char* buffer, uint32_t length);
void grid_usb_acm_rx_poll(struct grid_usb_acm_model* acm);
void grid_usb_acm_rx_process(struct grid_usb_acm_model* acm);

#endif /* GRID_USB_ACM_H */

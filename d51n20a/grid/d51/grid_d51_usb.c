#include "tusb.h"

#include "grid_d51_usb.h"

#include "grid_platform.h"
#include "grid_usb.h"

void usb_d_init(void) {} // required by ASF driver_init.c; replaced by TinyUSB

void USB_0_Handler(void) { dcd_int_handler(0); }
void USB_1_Handler(void) { dcd_int_handler(0); }
void USB_2_Handler(void) { dcd_int_handler(0); }
void USB_3_Handler(void) { dcd_int_handler(0); }

void grid_d51_usb_init(void) {
  uint32_t id[4] = {0};
  grid_platform_get_id(id);
  static char serial[33];
  grid_platform_id_to_hex(id, 16, serial);
  grid_usb_init(0x03eb, 0xecad, serial);
}

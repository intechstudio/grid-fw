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
  static const char hex[] = "0123456789abcdef";
  uint8_t* b = (uint8_t*)id;
  for (uint8_t i = 0; i < 16; i++) {
    serial[i * 2] = hex[(b[i] >> 4) & 0xf];
    serial[i * 2 + 1] = hex[b[i] & 0xf];
  }
  serial[32] = '\0'; // 128-bit unique ID
  grid_usb_init(0x03eb, 0xecad, serial);
}

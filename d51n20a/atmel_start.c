#include <atmel_start.h>

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void) {
  system_init();
  // usb_init() removed: TinyUSB is now initialized via grid_d51_usb_init()
  stdio_redirect_init();
}

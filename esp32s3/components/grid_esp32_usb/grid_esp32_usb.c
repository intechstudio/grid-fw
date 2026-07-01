#include "tusb.h"

#include "esp_err.h"
#include "esp_private/usb_phy.h"
#include "grid_esp32_usb.h"
#include "grid_platform.h"
#include "grid_usb.h"

void grid_esp32_usb_init(void) {

  usb_phy_handle_t phy_hdl;
  usb_phy_config_t phy_conf = {
      .controller = USB_PHY_CTRL_OTG,
      .target = USB_PHY_TARGET_INT,
      .otg_mode = USB_OTG_MODE_DEVICE,
      .otg_speed = USB_PHY_SPEED_FULL,
  };
  ESP_ERROR_CHECK(usb_new_phy(&phy_conf, &phy_hdl));

  uint32_t id[4] = {0};
  grid_platform_get_id(id);
  static char serial[13];
  grid_platform_id_to_hex(id, 6, serial);
  grid_usb_init(0x303a, 0x8123, serial);
}

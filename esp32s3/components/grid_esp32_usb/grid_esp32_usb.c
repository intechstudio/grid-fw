/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>

#include "tusb.h"

#include "esp_check.h"
#include "esp_log.h"

#include "esp_private/usb_phy.h"
#include "grid_esp32_usb.h"
#include "grid_usb.h"

static const char* TAG = "USB";

// ========================= USB Descriptors =============================== //

// Device descriptor.  We use the IAD class because CDC is enabled.
static const tusb_desc_device_t s_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    // IAD required when CDC is present
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303a,  // Espressif VID (same as sdkconfig CUSTOM_VID)
    .idProduct = 0x8123, // sdkconfig CUSTOM_PID
    .bcdDevice = 0x0100, // sdkconfig BCD_DEVICE
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

// Interface counter
enum interface_count {
#if CFG_TUD_CDC
  ITF_NUM_CDC_NOTIFY,
  ITF_NUM_CDC_DATA,
#endif
#if CFG_TUD_MIDI
  ITF_NUM_MIDI,
  ITF_NUM_MIDI_STREAMING,
#endif
#if CFG_TUD_HID
  ITF_NUM_HID,
#endif
  ITF_COUNT
};

// USB Endpoint numbers
enum usb_endpoints {
  EP_EMPTY = 0,
#if CFG_TUD_CDC
  EPNUM_CDC_NOTIFY,
  EPNUM_CDC_DATA,
#endif
#if CFG_TUD_MIDI
  EPNUM_MIDI,
#endif
#if CFG_TUD_HID
  EPNUM_HID,
#endif
  ENDPOINT_COUNT
};

#if CFG_TUD_HID
#define HID_DESC_LEN TUD_HID_DESC_LEN
#else
#define HID_DESC_LEN 0
#endif

#define TUSB_DESCRIPTOR_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN + CFG_TUD_HID * HID_DESC_LEN)

// String descriptors (index 0 = language ID as raw UTF-16LE bytes)
static const uint16_t _usb_lang_id[] = {0x0409};

static const void* s_str_desc[6] = {
    _usb_lang_id,              // 0: language ID
    "Intech Studio",           // 1: Manufacturer
    "Grid",                    // 2: Product
    "123456",                  // 3: Serial
    "Intech Grid MIDI device", // 4: MIDI interface
    "Intech Grid CDC device",  // 5: CDC interface
};

static const uint8_t strcnt = 6;

// Configuration descriptor
static uint8_t s_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 500),

#if CFG_TUD_CDC
    // Notification EP reduced to 8 bytes (notifications are small)
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 8, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),
#endif

#if CFG_TUD_MIDI
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
#endif

#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, false, GRID_HID_REPORT_DESC_SIZE, (0x80 | EPNUM_HID), 16, 10),
#endif
};

// ========================= TinyUSB Descriptor Callbacks =============================== //

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&s_device_desc; }

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // Only one configuration supported
  return s_cfg_desc;
}

#define MAX_DESC_STR_LEN 32

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  static uint16_t desc_str[MAX_DESC_STR_LEN];

  uint8_t chr_count;
  if (index == 0) {
    // Language ID: copy 2 raw bytes from the uint16_t array entry
    memcpy(&desc_str[1], s_str_desc[0], 2);
    chr_count = 1;
  } else if (index < strcnt) {
    const char* str = (const char*)s_str_desc[index];
    chr_count = (uint8_t)strnlen(str, MAX_DESC_STR_LEN - 1);
    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = str[i]; // ASCII → UTF-16LE
    }
  } else {
    return NULL;
  }

  // First word: length in bytes (header + char data) | descriptor type
  desc_str[0] = (TUSB_DESC_STRING << 8) | (uint8_t)(2 * chr_count + 2);
  return desc_str;
}

// ========================= USB Initialization =============================== //

void grid_esp32_usb_init(void) {

  // 1. Configure the on-chip USB PHY for full-speed OTG device mode.
  usb_phy_handle_t phy_hdl;
  usb_phy_config_t phy_conf = {
      .controller = USB_PHY_CTRL_OTG,
      .target = USB_PHY_TARGET_INT,
      .otg_mode = USB_OTG_MODE_DEVICE,
      .otg_speed = USB_PHY_SPEED_FULL,
  };
  ESP_ERROR_CHECK(usb_new_phy(&phy_conf, &phy_hdl));

  // 2. Initialise USB infrastructure (ACM, MIDI buffers, HID keyboard model)
  grid_usb_infrastructure_init();

  // 3. Initialise TinyUSB device stack
  tusb_init();

  ESP_LOGI(TAG, "USB initialized");
}

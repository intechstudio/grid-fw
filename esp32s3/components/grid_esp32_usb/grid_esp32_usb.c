/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_usb.h"

#include "esp_check.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

static const char* TAG = "USB";

// ========================= USB Descriptors =============================== //

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

/**
 * @brief String descriptor
 */
// UTF-16LE language ID (0x0409 = English US)
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

/**
 * @brief Configuration descriptor
 */
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
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, false, HID_REPORT_DESCRIPTOR_SIZE, (0x80 | EPNUM_HID), 16, 10),
#endif
};

// ========================= USB Initialization =============================== //

void grid_esp32_usb_init(void) {

  tinyusb_config_t config = TINYUSB_DEFAULT_CONFIG();
  config.descriptor.device = NULL;
  config.descriptor.string = (const char**)s_str_desc;
  config.descriptor.string_count = strcnt;
  config.phy.skip_setup = false;
  config.descriptor.full_speed_config = s_cfg_desc;

  ESP_ERROR_CHECK(tinyusb_driver_install(&config));

  // Initialize ACM subsystem (uses native tud_cdc_* callbacks)
  grid_esp32_usb_acm_init();

  ESP_LOGI(TAG, "USB initialized");
}

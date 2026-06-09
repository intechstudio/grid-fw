/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 *
 * USB functionality has been refactored into:
 *   grid_usb_midi.c  - MIDI buffer/TX/RX/SysEx/RTM
 *   grid_usb_hid.c   - HID keyboard model + gamepad + enable/disable
 */

#include "tusb.h"

#include "grid_usb.h"

#include <string.h>

#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_usb_desc.h"

struct grid_usb_model grid_usb_state = {0};

bool grid_usb_connected(void) { return tud_mounted(); }

void grid_usb_task(void) { tud_task_ext(0, false); }

void tud_mount_cb(void) {
  grid_usb_midi_on_connect(&grid_usb_state.midi);
  grid_usb_hid_on_connect(&grid_usb_state.hid);
  grid_usb_acm_on_connect(&grid_usb_state.acm);
}

void tud_umount_cb(void) {
  grid_usb_midi_on_disconnect(&grid_usb_state.midi);
  grid_usb_hid_on_disconnect(&grid_usb_state.hid);
  grid_usb_acm_on_disconnect(&grid_usb_state.acm);
}

static tusb_desc_device_t s_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0,
    .idProduct = 0,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x00,
    .bNumConfigurations = 0x01,
};

static const uint16_t s_lang_id[] = {0x0409};

static const void* s_str_table[] = {
    s_lang_id,          // 0: language ID
    "Intech Studio",    // 1: Manufacturer
    "Grid",             // 2: Product
    NULL,               // 3: Serial (set at init)
    "Intech Grid MIDI", // 4: MIDI interface
    "Intech Grid CDC",  // 5: CDC interface
};

#define STR_TABLE_COUNT ((uint8_t)(sizeof(s_str_table) / sizeof(s_str_table[0])))

#define GRID_USB_CFG_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

static const uint8_t s_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, GRID_USB_CFG_DESC_TOTAL_LEN, 0, 500),
#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 8, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),
#endif
#if CFG_TUD_MIDI
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
#endif
#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, GRID_HID_REPORT_DESC_SIZE, (0x80 | EPNUM_HID), 16, 10),
#endif
};

static uint8_t const* grid_usb_config_desc(void) { return s_cfg_desc; }

#define GRID_USB_STR_DESC_MAX_LEN 33

static uint16_t const* grid_usb_string_desc(const void** str_table, uint8_t count, uint8_t index) {
  static uint16_t desc_str[GRID_USB_STR_DESC_MAX_LEN];
  uint8_t chr_count;
  if (index == 0) {
    memcpy(&desc_str[1], str_table[0], 2);
    chr_count = 1;
  } else if (index < count && str_table[index] != NULL) {
    const char* str = (const char*)str_table[index];
    chr_count = (uint8_t)strnlen(str, GRID_USB_STR_DESC_MAX_LEN - 1);
    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = (uint16_t)str[i];
    }
  } else {
    return NULL;
  }
  desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * chr_count + 2u));
  return desc_str;
}

void grid_usb_init(uint16_t vid, uint16_t pid, const char* serial) {
  s_device_desc.idVendor = vid;
  s_device_desc.idProduct = pid;
  s_str_table[3] = serial;
  s_device_desc.iSerialNumber = (serial != NULL) ? 0x03 : 0x00;
  grid_usb_acm_init(&grid_usb_state.acm, GRID_PARAMETER_SPI_TRANSACTION_length * 2);
  grid_usb_midi_init(&grid_usb_state.midi, GRID_MIDI_TX_BUFFER_SIZE, GRID_MIDI_VOICE_RX_BUFFER_SIZE, GRID_MIDI_SYSEX_BUFFER_SIZE, GRID_MIDI_RTM_BUFFER_SIZE);
  grid_usb_hid_init(&grid_usb_state.hid);
  tusb_init();
}

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&s_device_desc; }

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return grid_usb_config_desc();
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  return grid_usb_string_desc(s_str_table, STR_TABLE_COUNT, index);
}

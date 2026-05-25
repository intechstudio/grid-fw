#include "grid_d51_usb.h"

#include <string.h>

#include "tusb.h"

#include "grid_d51_usb_acm.h"
#include "grid_usb.h"

static const tusb_desc_device_t s_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x03eb,
    .idProduct = 0xecad,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x00,
    .bNumConfigurations = 0x01,
};

enum { ITF_NUM_CDC_NOTIFY = 0, ITF_NUM_CDC_DATA, ITF_NUM_MIDI, ITF_NUM_MIDI_STREAMING, ITF_NUM_HID, ITF_COUNT };

enum {
  EP_EMPTY = 0,
  EPNUM_CDC_DATA,
  EPNUM_CDC_NOTIFY,
  EPNUM_MIDI,
  EPNUM_HID,
};

#define HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))

#define HID_REPORT_DESC_LEN sizeof((uint8_t[]){HID_REPORT_DESC_CONTENT})

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MIDI_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t s_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESC_TOTAL_LEN, 0, 250),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 8, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, HID_REPORT_DESC_LEN, (0x80 | EPNUM_HID), 16, 10),
};

static const uint16_t s_lang_id[] = {0x0409};

static const void* s_str_desc[] = {
    s_lang_id, "Intech Studio", "Grid", NULL, "Intech Studio: MS", "Intech Studio: CDC",
};

#define STR_DESC_COUNT ((uint8_t)(sizeof(s_str_desc) / sizeof(s_str_desc[0])))
#define MAX_STR_LEN 32

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&s_device_desc; }

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return s_cfg_desc;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  static uint16_t desc_str[MAX_STR_LEN];

  uint8_t chr_count;

  if (index == 0) {
    memcpy(&desc_str[1], s_str_desc[0], 2);
    chr_count = 1;
  } else if (index < STR_DESC_COUNT && s_str_desc[index] != NULL) {
    const char* str = (const char*)s_str_desc[index];
    chr_count = (uint8_t)strnlen(str, MAX_STR_LEN - 1);
    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = (uint16_t)str[i];
    }
  } else {
    return NULL;
  }

  desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * chr_count + 2u));
  return desc_str;
}

void USB_0_Handler(void) { dcd_int_handler(0); }
void USB_1_Handler(void) { dcd_int_handler(0); }
void USB_2_Handler(void) { dcd_int_handler(0); }
void USB_3_Handler(void) { dcd_int_handler(0); }

void usb_d_init(void) {}

void grid_d51_usb_init(void) {
  grid_d51_usb_acm_init();
  grid_usb_midi_buffer_init();
  grid_usb_keyboard_model_init(&grid_usb_keyboard_state, 100);
  tusb_init();
}

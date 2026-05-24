/*
 * grid_d51_usb.c — TinyUSB-based USB for Grid SAMD51 firmware.
 *
 * Replaces the former ASF4/Atmel START composite USB stack.
 *
 * Interface layout (full-speed, OPT_OS_NONE):
 *   0   CDC Communication (notify)
 *   1   CDC Data
 *   2   MIDI AudioControl
 *   3   MIDI MidiStreaming
 *   4   HID  (keyboard report 1 + mouse report 2)
 *
 * Endpoints:
 *   EP1 IN/OUT  CDC data bulk
 *   EP2 IN      CDC notify interrupt
 *   EP3 IN/OUT  MIDI bulk
 *   EP4 IN      HID interrupt
 */

#include "grid_d51_usb.h"

#include <assert.h>
#include <string.h>

#include "class/hid/hid_device.h"
#include "tusb.h"

#include "grid_msg.h"
#include "grid_port.h"
#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_transport.h"
#include "grid_usb.h"

// ============================================================
// USB Descriptors
// ============================================================

// ---- Device descriptor ----
// IAD class required because CDC is enabled alongside other interfaces.
static const tusb_desc_device_t s_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x03eb,  // Atmel / Microchip (same as ASF4 build)
    .idProduct = 0xecad, // same as ASF4 build
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x00, // no serial string
    .bNumConfigurations = 0x01,
};

// ---- Interface + endpoint numbers ----
enum { ITF_NUM_CDC_NOTIFY = 0, ITF_NUM_CDC_DATA, ITF_NUM_MIDI, ITF_NUM_MIDI_STREAMING, ITF_NUM_HID, ITF_COUNT };

enum {
  EP_EMPTY = 0,
  EPNUM_CDC_DATA,   // 1
  EPNUM_CDC_NOTIFY, // 2
  EPNUM_MIDI,       // 3
  EPNUM_HID,        // 4
};

// ---- HID report descriptor: keyboard (id=1) + mouse (id=2) ----
#define HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))

static const uint8_t s_hid_report_desc[] = {HID_REPORT_DESC_CONTENT};

#define HID_REPORT_DESC_LEN sizeof(s_hid_report_desc)

// ---- Configuration descriptor ----
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MIDI_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t s_cfg_desc[] = {
    // Configuration header (0 = bus-powered, no remote wakeup)
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESC_TOTAL_LEN, 0, 250),

    // CDC: notify EP tiny (8 bytes is enough for line-state notifications)
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 8, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),

    // MIDI
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),

    // HID (no OUT endpoint, boot-protocol = none)
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, HID_REPORT_DESC_LEN, (0x80 | EPNUM_HID), 16, 10),
};

// ---- String descriptors ----
// Index 0: language ID (raw UTF-16LE bytes)
static const uint16_t s_lang_id[] = {0x0409};

// Indices 1-5 are ASCII strings; index 4 = MIDI iface, index 5 = CDC iface.
static const void* s_str_desc[] = {
    s_lang_id,            // 0: language ID
    "Intech Studio",      // 1: manufacturer
    "Grid",               // 2: product
    NULL,                 // 3: serial (unused, iSerialNumber = 0)
    "Intech Studio: MS",  // 4: MIDI streaming interface
    "Intech Studio: CDC", // 5: CDC interface
};

#define STR_DESC_COUNT ((uint8_t)(sizeof(s_str_desc) / sizeof(s_str_desc[0])))
#define MAX_STR_LEN 32

// ============================================================
// TinyUSB descriptor callbacks
// ============================================================

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
    // Language ID: 2 raw bytes
    memcpy(&desc_str[1], s_str_desc[0], 2);
    chr_count = 1;
  } else if (index < STR_DESC_COUNT && s_str_desc[index] != NULL) {
    const char* str = (const char*)s_str_desc[index];
    chr_count = (uint8_t)strnlen(str, MAX_STR_LEN - 1);
    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = (uint16_t)str[i]; // ASCII → UTF-16LE
    }
  } else {
    return NULL;
  }

  // First word: total length in bytes | descriptor type
  desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * chr_count + 2u));
  return desc_str;
}

// ============================================================
// SAMD51 USB interrupt handlers → forward to TinyUSB
// ============================================================

void USB_0_Handler(void) { dcd_int_handler(0); }
void USB_1_Handler(void) { dcd_int_handler(0); }
void USB_2_Handler(void) { dcd_int_handler(0); }
void USB_3_Handler(void) { dcd_int_handler(0); }

// ============================================================
// HID report descriptor callback
// ============================================================

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return s_hid_report_desc;
}

// Invoked when received GET_REPORT — not used, return 0
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

// Invoked when received SET_REPORT — not used
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

// ============================================================
// CDC (virtual serial)
// ============================================================

static struct grid_swsr_t s_cdc_rx;
static uint8_t s_usb_tx_ready = 0;

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;

  uint8_t buf[512];
  uint32_t rx_size = tud_cdc_read(buf, sizeof(buf));

  if (rx_size == 0) {
    return;
  }

  struct grid_swsr_t* rx = &s_cdc_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    // Overflow: discard accumulated data and accept fresh data
    grid_swsr_read(rx, NULL, grid_swsr_size(rx));
    grid_swsr_write(rx, buf, rx_size);
  }

  struct grid_msg msg;

  if (!grid_msg_from_swsr(&msg, rx)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) == 0) {
    grid_transport_recv_usb(&grid_transport_state, (uint8_t*)msg.data, msg.length);
  }
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
  (void)itf;
  s_usb_tx_ready = 1;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;
  if (dtr) {
    s_usb_tx_ready = 1;
  }
}

int32_t grid_platform_usb_serial_ready(void) { return s_usb_tx_ready; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {
  if (s_usb_tx_ready) {
    s_usb_tx_ready = 0;
    tud_cdc_write(buffer, length);
    tud_cdc_write_flush();
  } else {
    tud_cdc_write_flush();
  }
  return 1;
}

// ============================================================
// MIDI
// ============================================================

void grid_d51_midi_rx_poll(void) {
  uint8_t packet[4];

  while (tud_midi_available()) {
    if (!grid_midi_rx_writable()) {
      break;
    }
    if (tud_midi_packet_read(packet)) {
      grid_midi_rx_push(packet[0], packet[1], packet[2], packet[3]);
    }
  }
}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  const uint8_t buf[4] = {byte0, byte1, byte2, byte3};
  if (tud_midi_mounted()) {
    tud_midi_packet_write(buf);
  }
  return 0;
}

int32_t grid_platform_usb_midi_write_status(void) { return 0; }

// ============================================================
// HID — Mouse
// ============================================================

static uint8_t s_mouse_buttons = 0;

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) {
  if (b_state) {
    s_mouse_buttons |= type;
  } else {
    s_mouse_buttons &= (uint8_t)~type;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, s_mouse_buttons, 0, 0, 0, 0);
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) {
  int8_t dx = 0, dy = 0, wheel = 0, pan = 0;

  switch (axis) {
  case 0x01:
    dx = position;
    break; // X_AXIS_MV
  case 0x02:
    dy = position;
    break; // Y_AXIS_MV
  case 0x03:
    wheel = position;
    break; // SCROLL_MV
  default:
    return 0;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, s_mouse_buttons, dx, dy, wheel, pan);
}

// ============================================================
// HID — Gamepad (not supported on D51)
// ============================================================

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) {
  (void)axis;
  (void)value;
  return -1; // not supported
}

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) {
  (void)button;
  (void)value;
  return -1; // not supported
}

// ============================================================
// HID — Keyboard
// ============================================================

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {

  uint8_t keycode[6] = {0};
  uint8_t modifier = 0;
  uint8_t key_idx = 0;

  for (uint8_t i = 0; i < keys_count && i < GRID_KEYBOARD_KEY_maxcount; i++) {
    if (active_key_list[i].ismodifier) {
      modifier |= active_key_list[i].keycode;
    } else if (active_key_list[i].ispressed && active_key_list[i].keycode != 255 && key_idx < 6) {
      keycode[key_idx++] = active_key_list[i].keycode;
    }
  }

  return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}

// ============================================================
// WebSocket stubs (D51 has no network)
// ============================================================

int32_t grid_platform_websocket_ready(void) { return 0; }

int32_t grid_platform_websocket_write(char* buffer, uint32_t length) {
  (void)buffer;
  (void)length;
  return 0;
}

// ============================================================
// ASF4 USB HAL stub
//
// driver_init.c calls usb_d_init() from USB_DEVICE_INSTANCE_init().
// The USB clock and pin-mux setup that surrounds it (CLOCK_init /
// PORT_init) still runs and is needed by TinyUSB.  Only usb_d_init()
// itself is replaced by a no-op here; TinyUSB initialises the USB
// peripheral registers via dcd_samd.c when tusb_init() is called.
// ============================================================

void usb_d_init(void) {
  // Intentionally empty: TinyUSB owns the USB peripheral from here.
}

// ============================================================
// Initialisation
// ============================================================

void grid_d51_usb_init(void) {
  // Allocate CDC RX ring buffer
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&s_cdc_rx, capacity) == 0);

  // Allocate MIDI TX/RX ring buffers (used by grid_midi_rx_push etc.)
  grid_usb_midi_buffer_init();

  // Keyboard model
  grid_usb_keyboard_model_init(&grid_usb_keyboard_state, 100);

  // Start TinyUSB (bare-metal, no RTOS)
  tusb_init();
}

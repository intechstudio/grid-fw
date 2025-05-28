/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_usb.h"

#include "esp_check.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "tinyusb.h"

// #include "../../grid_common/grid_port.h"
#include "grid_transport.h"
#include "grid_usb.h"
// #include "../../grid_common/grid_sys.h"

#include "driver/gpio.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static const char* TAG = "USB example";

void tud_midi_rx_cb(uint8_t itf) {

  (void)itf;

  // ets_printf("MIDI RX: %d\n", itf);

  // The MIDI interface always creates input and output port/jack descriptors
  // regardless of these being used or not. Therefore incoming traffic should be
  // read (possibly just discarded) to avoid the sender blocking in IO
  uint8_t packet[4];
  bool read = false;

  while (tud_midi_available()) {

    if (!grid_midi_rx_writable()) {
      break;
    }

    if (tud_midi_packet_read(packet)) {

      // ets_printf("Read, Data: %02x %02x %02x %02x\r\n", packet[0], packet[1], packet[2], packet[3]);

      uint8_t channel = packet[1] & 0x0f;
      uint8_t command = packet[1] & 0xf0;
      uint8_t param1 = packet[2];
      uint8_t param2 = packet[3];

      // grid_port_debug_printf("decoded: %d %d %d %d", channel, command,
      // param1, param2);

      struct grid_midi_event_desc midi_ev;

      midi_ev.byte0 = channel;
      midi_ev.byte1 = command;
      midi_ev.byte2 = param1;
      midi_ev.byte3 = param2;

      grid_midi_rx_push(midi_ev);
    }
  }
}

struct grid_swsr_t cdc_rx;

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t* event) {

  size_t rx_size = 0;
  uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  esp_err_t err = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);

  if (err != ESP_OK) {
    return;
  }

  struct grid_swsr_t* rx = &cdc_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    grid_swsr_read(rx, NULL, grid_swsr_size(rx));
  }

  int ret = grid_swsr_cspn(rx, '\n');

  if (ret < 0) {
    return;
  }

  assert(ret < GRID_PARAMETER_SPI_TRANSACTION_length);
  uint8_t temp[GRID_PARAMETER_SPI_TRANSACTION_length + 1];

  assert(grid_swsr_readable(rx, ret + 1));
  grid_swsr_read(rx, temp, ret + 1);
  temp[ret + 1] = '\0';

  if (grid_str_verify_frame((char*)temp, ret + 1)) {
    return;
  }

  grid_transport_recv_usb(&grid_transport_state, temp, ret + 1);
}

static uint8_t DRAM_ATTR usb_tx_ready = 0;

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t* event) {
  int dtr = event->line_state_changed_data.dtr;
  int rts = event->line_state_changed_data.rts;
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);

  usb_tx_ready = 1;
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
  // ets_printf("CDC TXC\r\n");
  /*esp_err_t status = */ tinyusb_cdcacm_write_flush(0, 0);

  usb_tx_ready = 1;
  // ets_printf("# %d\r\n", status);
}

int32_t grid_platform_usb_serial_ready() { return usb_tx_ready; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) {

  // portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
  // portENTER_CRITICAL(&spinlock);

  esp_err_t status = 0;

  // tinyusb_cdcacm_write_flush(0, pdMS_TO_TICKS(10));
  if (usb_tx_ready == 1) {

    // ets_printf("$\r\n");

    usb_tx_ready = 0;

    uint32_t queued = tinyusb_cdcacm_write_queue(0, (const uint8_t*)buffer, length);

    if (queued != length) {
      ets_printf("CDC QUEUE ERROR: %d %d\r\n", queued, length);
      tinyusb_cdcacm_write_flush(0, 0);
    } else {
      status = tinyusb_cdcacm_write_flush(0, 0);
      // ets_printf("$ %d\r\n", status);
    }
  } else {

    status = tinyusb_cdcacm_write_flush(0, 0);

    // ets_printf("SKIP %d\r\n", status);

    if (status == ESP_OK) {
      // ets_printf("READY\r\n");
      usb_tx_ready = 1;
    }
  }

  // portEXIT_CRITICAL(&spinlock);

  return 1;
}

// =========================== HID ======================== //

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)),
                                         TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  // We use only one interface and one HID report descriptor, so we can ignore
  // parameter 'instance'
  return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

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
  // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
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

// #EPNUM_MIDI_IN
// #EPNUM_MIDI_OUT

/** TinyUSB descriptors **/

#define TUSB_DESCRIPTOR_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief String descriptor
 */
static const char* s_str_desc[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},      // 0: is supported language is English (0x0409)
    "Intech Studio",           // 1: Manufacturer
    "Grid",                    // 2: Product
    "123456",                  // 3: Serials, should use chip ID
    "Intech Grid MIDI device", // 4: MIDI
    "Intech Grid CDC device",  // 5: CDC
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and a
 * MIDI interface
 */
static uint8_t s_cfg_desc[] = {
    // Configuration number, interface count, string index, total length,
    // attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 500),

#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 64, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),
#endif

#if CFG_TUD_MIDI
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
#endif

#if CFG_TUD_HID
    // Interface number, string index, boot protocol, report descriptor len, EP
    // In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, false, sizeof(hid_report_descriptor), (0x80 | EPNUM_HID), 16, 10),
#endif

};

void grid_esp32_usb_init() {

  tinyusb_config_t tusb_cfg = {
      .device_descriptor = NULL, // If device_descriptor is NULL,
                                 // tinyusb_driver_install() will use Kconfig
      .string_descriptor = s_str_desc,
      .external_phy = false,
      .configuration_descriptor = s_cfg_desc,
  };

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  tinyusb_config_cdcacm_t acm_cfg = {.usb_dev = TINYUSB_USBDEV_0,
                                     .cdc_port = TINYUSB_CDC_ACM_0,
                                     .rx_unread_buf_sz = 64,
                                     .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
                                     .callback_rx_wanted_char = NULL,
                                     .callback_line_state_changed = NULL,
                                     .callback_line_coding_changed = NULL};

  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
  /* the second way to register a callback */
  ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_0, CDC_EVENT_LINE_STATE_CHANGED, &tinyusb_cdc_line_state_changed_callback));

  // Allocate CDC RX buffer
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&cdc_rx, capacity) == 0);

  // END OF USB
}

void grid_esp32_usb_task(void* arg) {

  grid_esp32_usb_init();
  grid_usb_midi_buffer_init();
  grid_usb_keyboard_model_init(&grid_usb_keyboard_state, 100);

  ESP_LOGD(TAG, "tinyusb task started");
  while (1) { // RTOS forever loop

    tud_task();

    // Duplicate midi rx callback used as a polling mechanism, as the
    // actual callback may not necessarily process all available data
    tud_midi_rx_cb(0);

    taskYIELD();
  }
}

// ========================= PLATFORM =============================== //

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  const uint8_t buffer[] = {byte0, byte1, byte2, byte3};

  if (tud_midi_mounted()) {

    tud_midi_packet_write(buffer);

    // tud_midi_stream_write(0, &buffer[1], 3);

    // ets_printf("MIDI\r\n");
  }

  return 0;
}

int32_t grid_platform_usb_midi_write_status(void) {

  // ets_printf("PLATFORM MIDI STATUS \r\n");
  // ets_printf("grid_platform_usb_midi_write_status NOT IMPLEMENTED!!!");
  return 0;
}

enum mouse_button_type { LEFT_BTN = 0x01, RIGHT_BTN = 0x02, MIDDLE_BTN = 0x04 };

static uint8_t hid_mouse_button_state = 0;

static uint32_t hid_gamepad_button_state = 0;

static int8_t hid_gamepad_axis_x = 0;
static int8_t hid_gamepad_axis_y = 0;
static int8_t hid_gamepad_axis_z = 0;

static int8_t hid_gamepad_axis_rx = 0;
static int8_t hid_gamepad_axis_ry = 0;
static int8_t hid_gamepad_axis_rz = 0;

static uint8_t hid_gamepad_hat = 0;

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) {

  if (b_state == 1) { // button pressed
    hid_mouse_button_state |= type;
  } else {
    hid_mouse_button_state &= ~type;
  }

  // report_id, buttons, dx, dy, wheel, pan
  tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, hid_mouse_button_state, 0, 0, 0, 0);

  return 1;
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) {

  int8_t delta_x = 0;
  int8_t delta_y = 0;
  int8_t wheel = 0;
  int8_t pan = 0; // not used

  if (axis == X_AXIS_MV) {
    delta_x = position;
  } else if (axis == Y_AXIS_MV) {
    delta_y = position;
  } else if (axis == SCROLL_MV) {
    wheel = position;
  } else {
    return 0;
  }

  // report_id, buttons, dx, dy, wheel, pan
  tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, hid_mouse_button_state, delta_x, delta_y, wheel, pan);

  return 1;
}

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) {

  switch (axis) {
  case GAMEPAD_AXIS_X:
    hid_gamepad_axis_x = value;
    break;
  case GAMEPAD_AXIS_Y:
    hid_gamepad_axis_y = value;
    break;
  case GAMEPAD_AXIS_Z:
    hid_gamepad_axis_z = value;
    break;
  case GAMEPAD_AXIS_RX:
    hid_gamepad_axis_rx = value;
    break;
  case GAMEPAD_AXIS_RY:
    hid_gamepad_axis_ry = value;
    break;
  case GAMEPAD_AXIS_RZ:
    hid_gamepad_axis_rz = value;
    break;
  default:
    ets_printf("INVALID AXIS\r\n");
    return 0;
  }

  tud_hid_gamepad_report(3, hid_gamepad_axis_x, hid_gamepad_axis_y, hid_gamepad_axis_z, hid_gamepad_axis_rz, hid_gamepad_axis_ry, hid_gamepad_axis_rx, hid_gamepad_hat, hid_gamepad_button_state);

  return 1;
}

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) {

  if (value) {
    hid_gamepad_button_state |= (1 << button);
  } else {
    hid_gamepad_button_state &= ~(1 << button);
  }

  tud_hid_gamepad_report(3, hid_gamepad_axis_x, hid_gamepad_axis_y, hid_gamepad_axis_z, hid_gamepad_axis_rx, hid_gamepad_axis_ry, hid_gamepad_axis_rz, hid_gamepad_hat, hid_gamepad_button_state);

  return 1;
}

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {

  struct grid_usb_hid_kb_desc key_descriptor_array[GRID_KEYBOARD_KEY_maxcount];
  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {

    key_descriptor_array[i].b_modifier = active_key_list[i].ismodifier;
    key_descriptor_array[i].key_id = active_key_list[i].keycode;
    key_descriptor_array[i].state = active_key_list[i].ispressed;
  }

  uint8_t keycode[6] = {0};

  uint8_t modifier = 0; // modifier flags

  if (keys_count == 0) {
    ESP_LOGD(TAG, "No Key Is Pressed");
  }

  for (uint8_t i = 0; i < keys_count; i++) {

    ESP_LOGD(TAG, "IsMod: %d, KeyCode: %d, State: %d", key_descriptor_array[i].b_modifier, key_descriptor_array[i].key_id, key_descriptor_array[i].state);

    if (key_descriptor_array[i].b_modifier) {

      modifier |= key_descriptor_array[i].key_id;
    } else if (key_descriptor_array[i].state && key_descriptor_array[i].key_id != 255) {

      keycode[keys_count] = key_descriptor_array[i].key_id;
      keys_count++;
    }
  }

  // Report Id, modifier, keycodearray
  tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);

  return 0;
}

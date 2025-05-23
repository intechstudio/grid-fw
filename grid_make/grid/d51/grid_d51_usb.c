/*
 * grid_d51_usb.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */

#include "grid_d51_usb.h"

#include <assert.h>

volatile uint8_t grid_usb_serial_rx_buffer[CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ];

volatile uint8_t grid_usb_serial_rx_flag;
volatile uint16_t grid_usb_serial_rx_size;

static volatile struct grid_port* host_port = NULL;

struct grid_swsr_t usb_rx;

static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  size_t rx_size = count;
  uint8_t* buf = grid_usb_serial_rx_buffer;

  struct grid_swsr_t* rx = &usb_rx;

  if (grid_swsr_writable(rx, rx_size)) {
    grid_swsr_write(rx, buf, rx_size);
  } else {
    grid_swsr_read(rx, NULL, grid_swsr_size(rx));
  }

  int ret = grid_swsr_cspn(rx, '\n');

  if (ret < 0) {
    goto bulkout_cb_end;
  }

  assert(ret < GRID_PARAMETER_SPI_TRANSACTION_length);
  uint8_t temp[GRID_PARAMETER_SPI_TRANSACTION_length + 1];

  assert(grid_swsr_readable(rx, ret + 1));
  grid_swsr_read(rx, temp, ret + 1);
  temp[ret + 1] = '\0';

  if (grid_str_verify_frame((char*)temp, ret + 1)) {
    goto bulkout_cb_end;
  }

  grid_transport_recv_usb(&grid_transport_state, temp, ret + 1);

bulkout_cb_end:

  cdcdf_acm_read((uint8_t*)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));

  return false;
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);

  return false; /* No error. */
}
static uint8_t usb_tx_ready = 0;

static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state) {

  // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);

  // printf("\r\n### USB SERIAL STATE CHANGE %d ###\r\n",
  // sizeof(grid_usb_serial_rx_buffer));

  if (state.rs232.DTR || 1) {
    /* After connection the R/W callbacks can be registered */
    cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_serial_bulkout_cb);
    cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_serial_bulkin_cb);
    /* Start Rx */
    cdcdf_acm_read((uint8_t*)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));

    usb_tx_ready = 1;
  }

  return false; /* No error. */
}

int32_t grid_platform_usb_serial_ready() { return usb_tx_ready; }

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) { return cdcdf_acm_write((uint8_t*)buffer, length); }

enum { MIDI_RX_BUFFER_SIZE = 128 };

static uint8_t midi_rx_buffer[MIDI_RX_BUFFER_SIZE] = {0};

static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  for (uint32_t i = 0; i < count; i += 4) {

    if (i >= MIDI_RX_BUFFER_SIZE) {
      break;
    }

    if (i + 3 >= count) {
      break;
    }

    struct grid_midi_event_desc midi_ev;

    midi_ev.byte0 = midi_rx_buffer[i + 1] & 0x0f; // channel
    midi_ev.byte1 = midi_rx_buffer[i + 1] & 0xf0; // command
    midi_ev.byte2 = midi_rx_buffer[i + 2];        // param1
    midi_ev.byte3 = midi_rx_buffer[i + 3];        // param2

    if ((midi_ev.byte0 == 8 || midi_ev.byte0 == 10 || midi_ev.byte0 == 12) && midi_ev.byte1 == 240) {
      // if element's timer clock source is midi then decrement timer_helper
      grid_platform_sync1_pulse_send();
    }

    grid_midi_rx_push(midi_ev);

    memset(&midi_rx_buffer[i], 0, 4);
  }

  audiodf_midi_read(midi_rx_buffer, MIDI_RX_BUFFER_SIZE);

  return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  // printf("MIDI IN CB\n");
  // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
  return false;
}

static bool grid_usb_midi_installed_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  printf("MIDI INSTALLED CB\n");
  audiodf_midi_read(midi_rx_buffer, MIDI_RX_BUFFER_SIZE);
  return false;
}

void grid_d51_usb_init(void) {

  // Allocate USB RX buffer
  int capacity = GRID_PARAMETER_SPI_TRANSACTION_length * 2;
  assert(grid_swsr_malloc(&usb_rx, capacity) == 0);

  audiodf_midi_init();
  composite_device_start();

  // host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);

  // audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ,
  // (FUNC_PTR)midi_in_handler);
  // audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE,
  // (FUNC_PTR)midi_out_handler);

  grid_usb_serial_rx_size = 0;
  grid_usb_serial_rx_flag = 0;

  // this does not directly register the statechange callback to an endpoint,
  // just to the internal driver
  cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)grid_usb_serial_statechange_cb);

  audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ, (FUNC_PTR)grid_usb_midi_bulkin_cb);
  audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkout_cb);

  audiodf_midi_register_callback(AUDIODF_MIDI_CB_INSTALLED, (FUNC_PTR)grid_usb_midi_installed_cb);

  grid_usb_midi_buffer_init();

  grid_usb_keyboard_model_init(&grid_usb_keyboard_state, 100);
}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) { return audiodf_midi_write(byte0, byte1, byte2, byte3); }

int32_t grid_platform_usb_midi_write_status(void) { return audiodf_midi_write_status(); }

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) { return hiddf_mouse_button_change(b_state, type); }

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) { return hiddf_mouse_move(position, axis); }

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) { grid_port_debug_printf("Gamepad Not Supported"); }

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) { grid_port_debug_printf("Gamepad Not Supported"); }

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {

  struct grid_usb_hid_kb_desc hid_key_array[GRID_KEYBOARD_KEY_maxcount];
  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {

    hid_key_array[i].b_modifier = active_key_list[i].ismodifier;
    hid_key_array[i].key_id = active_key_list[i].keycode;
    hid_key_array[i].state = active_key_list[i].ispressed;
  }

  return hiddf_keyboard_keys_state_change(hid_key_array, keys_count);
}

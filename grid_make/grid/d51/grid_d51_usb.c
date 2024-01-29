/*
 * grid_d51_usb.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */

#include "grid_d51_usb.h"

static volatile struct grid_port* host_port = NULL;

static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  // printf("\r\n$ %d ", count);

  uint8_t halfpacket = 0;
  if (grid_usb_serial_rx_buffer[0] != GRID_CONST_SOH || grid_usb_serial_rx_buffer[count - 1] != 10) {
    halfpacket = 1;
    // printf("halfpacket");
  }

  for (uint16_t i = 0; i < count; i++) {

    // if (halfpacket && (i<10 || i>count-10)){
    // 	printf(" %02x", grid_usb_serial_rx_buffer[i]);
    // }
    // else if (halfpacket && i == 10){
    // 	printf(" ...");
    // }

    struct grid_doublebuffer* doublebuffer_rx = &grid_transport_state.doublebuffer_rx_array[5];
    doublebuffer_rx->buffer_storage[doublebuffer_rx->write_index] = grid_usb_serial_rx_buffer[i];

    // printf("%d, ", grid_usb_serial_rx_buffer[i]);

    doublebuffer_rx->write_index++;
    doublebuffer_rx->write_index %= doublebuffer_rx->buffer_size;
  }

  // CLEAR THE ENTIRE BUFFER
  for (uint16_t i = 0; i < sizeof(grid_usb_serial_rx_buffer); i++) {
    grid_usb_serial_rx_buffer[i] = 0;
  }

  cdcdf_acm_read((uint8_t*)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));

  // cdcdf_acm_write(cdcdf_demo_buf, count); /* Echo data */
  return false; /* No error. */
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);

  return false; /* No error. */
}
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
  }

  return false; /* No error. */
}

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length) { return cdcdf_acm_write((uint8_t*)buffer, length); }

static uint8_t midi_rx_buffer[4] = {0};

static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  struct grid_midi_event_desc midi_ev;

  midi_ev.byte0 = midi_rx_buffer[1] & 0x0f; // channel
  midi_ev.byte1 = midi_rx_buffer[1] & 0xf0; // command
  midi_ev.byte2 = midi_rx_buffer[2];        // param1
  midi_ev.byte3 = midi_rx_buffer[3];        // param2

  // printf("MIDI OUT CB %d %d %d %d \n", midi_ev.byte0, midi_ev.byte1,
  // midi_ev.byte2, midi_ev.byte3);

  if ((midi_ev.byte0 == 8 || midi_ev.byte0 == 10 || midi_ev.byte0 == 12) && midi_ev.byte1 == 240) {
    // if element's timer clock source is midi then decrement timer_helper
    grid_platform_sync1_pulse_send();
  }

  grid_midi_rx_push(midi_ev);

  for (uint8_t i = 0; i < 4; i++) {

    midi_rx_buffer[i] = 0;
  }

  audiodf_midi_read(midi_rx_buffer, 4);

  return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  // printf("MIDI IN CB\n");
  // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
  return false;
}

static bool grid_usb_midi_installed_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count) {

  printf("MIDI INSTALLED CB\n");
  audiodf_midi_read(midi_rx_buffer, 4);
  return false;
}

void grid_d51_usb_init(void) {

  audiodf_midi_init();
  composite_device_start();

  host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);

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

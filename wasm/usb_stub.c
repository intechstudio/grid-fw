// Stand-in for the real USB HID/MIDI/CDC transport (esp32s3/rp2040/d51n20a
// wire grid_usb_* up to tinyusb). grid_port.c and grid_decode.c reference
// it directly to forward *external* protocol messages (received from other
// modules) onto USB - this single-module simulator has no real USB device,
// so these just report success and drop the data.
//
// This is a separate path from a module's own grid_send()/midi_send()/etc.
// Lua calls, which go through grid_lua_append_stdo() instead and are
// readable via grid_sim_drain_stdo()/drain_stde() in sim_core.c.

#include <stdbool.h>
#include <stdint.h>

#include "grid_usb.h"
#include "grid_usb_gamepad.h"
#include "grid_usb_macro.h"
#include "grid_usb_midi.h"

struct grid_usb_model grid_usb_state = {0};

bool grid_usb_acm_tx_busy(struct grid_usb_acm_model* acm) { return false; }

int32_t grid_usb_acm_write(struct grid_usb_acm_model* acm, char* buffer, uint32_t length) { return (int32_t)length; }

uint8_t grid_usb_midi_tx_push(struct grid_usb_midi_model* midi, struct grid_midi_event_desc midi_event) { return 0; }

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* macro, struct grid_macro_event_desc event) { return 0; }

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* gamepad, enum gamepad_axis_t axis, int32_t value) { return 0; }

int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* gamepad, uint8_t button, uint8_t value) { return 0; }

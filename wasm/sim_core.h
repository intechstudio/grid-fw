#ifndef WASM_SIM_CORE_H
#define WASM_SIM_CORE_H

#include <stdint.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

// Shared simulator core for a single VSN1 (VSN1L) Grid module - the real
// grid_ui/grid_led/grid_lua/grid_config logic that ships to hardware,
// running headless. Both the browser SDL front-end (gui_main.c) and the
// Node/browser-library front-end (headless_main.c) drive the module through
// this API; nothing here depends on SDL or emscripten's main loop.
//
// Element layout (matches grid_module_vsnl_ui_init, common/src/c/grid_module.c):
//   0-7   button
//   8     endless potentiometer (rotation + integrated push button)
//   9-12  button
//   13    lcd (320x240, rendered into the buffer returned by
//         grid_sim_get_lcd_framebuffer())
//   14    system

#define GRID_SIM_ELEMENT_COUNT 15
#define GRID_SIM_ENDLESS_ELEMENT 8
#define GRID_SIM_LCD_ELEMENT 13
#define GRID_SIM_SYSTEM_ELEMENT 14

#define GRID_SIM_LCD_WIDTH 320
#define GRID_SIM_LCD_HEIGHT 240
#define GRID_SIM_LCD_BYTES_PER_PIXEL 3

// Transport port indices, matching the layout grid_sim_init() sets up in
// grid_transport_state.ports[] (common/src/c/grid_transport.h) - the same
// 6-port layout real pbf4/vsnx-family modules use.
#define GRID_SIM_PORT_NORTH 0
#define GRID_SIM_PORT_EAST 1
#define GRID_SIM_PORT_SOUTH 2
#define GRID_SIM_PORT_WEST 3
#define GRID_SIM_PORT_UI 4
#define GRID_SIM_PORT_USB 5
#define GRID_SIM_PORT_COUNT 6

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the module (elements, LEDs, LCD, Lua VM, default config). Call
// once before anything else. Safe to call again to reset the simulation.
EMSCRIPTEN_KEEPALIVE void grid_sim_init(void);

// Parse a config.toml, in the exact format the real module reads and writes
// on hardware, assigning custom Lua scripts to element events. Returns 0 on
// success, matching grid_config_parse's return value.
EMSCRIPTEN_KEEPALIVE int grid_sim_load_config(const char* toml);

// Advance the simulation's virtual clock by dt_ms milliseconds, running any
// due LED animation ticks, firing any triggered element events (running
// their Lua scripts), and draining the LCD's pending draw queue. Call this
// on every simulated frame, after feeding this frame's inputs.
EMSCRIPTEN_KEEPALIVE void grid_sim_tick(uint32_t dt_ms);

// Press/release a button element (0-7, 9-12). No-op for any other element.
EMSCRIPTEN_KEEPALIVE void grid_sim_input_button(uint8_t element, uint8_t pressed);

// Rotate the endless element (8) by `delta` quadrature steps (positive =
// clockwise). No-op for any other element.
EMSCRIPTEN_KEEPALIVE void grid_sim_input_endless_rotate(uint8_t element, int16_t delta);

// Press/release the endless element's (8) integrated push button. No-op for
// any other element.
EMSCRIPTEN_KEEPALIVE void grid_sim_input_endless_button(uint8_t element, uint8_t pressed);

// Element type, using the GRID_PARAMETER_ELEMENT_* values from
// common/src/c/grid_protocol.h (2 = button, 4 = endless, 5 = lcd, 0 = system).
EMSCRIPTEN_KEEPALIVE uint8_t grid_sim_get_element_type(uint8_t element);

// Current rendered color of one physical LED (0..17 on VSN1L), as plain RGB.
// Element index is NOT the same as physical LED index (see
// grid_sim_get_element_led_indices below) - use that to find which LED(s)
// to read here for a given element.
EMSCRIPTEN_KEEPALIVE void grid_sim_get_led_rgb(uint8_t num, uint8_t* r, uint8_t* g, uint8_t* b);
EMSCRIPTEN_KEEPALIVE uint8_t grid_sim_get_led_count(void);

// Which physical LED(s) light up for this element, per the module's own
// grid_led_lookup_alloc_* table (set up in grid_module_vsnl_ui_init,
// common/src/c/grid_module.c - e.g. button element 0's LED is physical LED
// 10, not LED 0). Writes up to max_out indices into out and returns how
// many were written; returns 0 for an element with no LED of its own
// (elements 9-12 and 13/14 on VSN1L).
EMSCRIPTEN_KEEPALIVE uint8_t grid_sim_get_element_led_indices(uint8_t element, uint8_t* out, uint8_t max_out);

// Convenience, pointer-free variants of the above two, for callers (plain
// HTML/JS pages) that don't want to deal with HEAPU8/malloc - a single
// 0x00RRGGBB value instead of writing through r/g/b pointers.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_get_led_rgb_packed(uint8_t num);
// This element's own LED color (its first physical LED, per
// grid_sim_get_element_led_indices) - 0 if it has none.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_get_element_led_rgb_packed(uint8_t element);

// Pointer to the LCD's live RGB888 framebuffer
// (GRID_SIM_LCD_WIDTH * GRID_SIM_LCD_HEIGHT * GRID_SIM_LCD_BYTES_PER_PIXEL
// bytes, row-major). Valid for the lifetime of the simulation; read it via
// HEAPU8 from JS after each grid_sim_tick().
EMSCRIPTEN_KEEPALIVE uint8_t* grid_sim_get_lcd_framebuffer(void);

// Drain (and clear) whatever the module has sent to its "UI" transport port
// since the last call - this is where grid_send()/midi_send()/keyboard_send()/
// etc. (common/src/c/grid_lua_api.c) end up: grid_ui_process_triggered()
// folds a triggered event's Lua stdo into a protocol message and hands it to
// grid_transport_send_msg_to_all(), which queues it on the port a real
// desktop editor reads from over USB. This is the correct place to observe
// a module's output - the Lua stdo/stde buffers themselves get cleared
// again internally on every processed event, before a caller of this API
// could ever read them. Returns raw protocol frame bytes (not necessarily
// UTF-8/printable); an empty string when nothing is pending.
EMSCRIPTEN_KEEPALIVE const char* grid_sim_drain_module_output(void);

// Drain (and clear) the Lua stdo/stde buffers directly. Only useful for
// output/errors produced *outside* the normal triggered-event pipeline (e.g.
// during grid_sim_load_config()) - during normal operation these are almost
// always empty, since grid_ui_event_render_events() (common/src/c/grid_ui.c)
// clears them on every event it processes, before this can run. Prefer
// grid_sim_drain_module_output() for what the module actually sent; a Lua
// error is also always echoed to the console regardless (grid_lua_broadcast_stde).
EMSCRIPTEN_KEEPALIVE const char* grid_sim_drain_stdo(void);
EMSCRIPTEN_KEEPALIVE const char* grid_sim_drain_stde(void);

// ---------------------------------------------------------------------------
// Port-level transport: rx -> broadcast-relay -> tx across all 6 ports
// (N/E/S/W USART, UI, USB), matching real firmware's per-tick loop (see
// d51n20a/grid_d51n20a.c's main() - the one target that actually wires this
// up in full: grid_transport_rx_broadcast_tx() per port, then
// grid_port_send_usart()/send_usb()/send_ui()). Lets you inject a message
// as if it just arrived over a port's wire/USB, and listen to whatever the
// module then queues to send back out that same port.
// ---------------------------------------------------------------------------

// Build a real DCT ("direct") handshake ping frame - the exact bytes
// grid_ping_init() (common/src/c/grid_port.c) produces for a module
// announcing itself as attached via `claimed_source_dir`. Inject this into
// a USART port (grid_sim_port_inject) to establish its connection before
// grid_sim_tick()'s relay will touch it - a BRC (broadcast) frame is
// ignored on an unconnected port, exactly like real hardware
// (grid_port_recv_msg, common/src/c/grid_port.c). Writes up to max_out
// bytes into out and returns how many were written (always <= 15).
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_build_ping_frame(uint8_t claimed_source_dir, char* out, uint32_t max_out);

// Builds a complete, valid BRC (broadcast) message wrapping one MIDI class
// frame (GRID_CLASS_MIDI_frame, common/src/c/grid_protocol.h), using the
// same steps real code takes to send one (l_grid_midi_send(), common/src/c/
// grid_lua_api.c, builds the class frame; grid_ui_clear_triggered(),
// grid_ui.c, wraps it in a BRC envelope and closes it). Ready to inject
// as-is via grid_sim_port_inject() (e.g. into GRID_SIM_PORT_USB) - already
// includes the checksum footer, unlike grid_sim_terminate_frame(). A
// concrete, known-correct example of what a real injectable message looks
// like end to end. Writes up to max_out bytes into out and returns how many.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_build_midi_frame(uint8_t channel, uint8_t command, uint8_t param1, uint8_t param2, char* out, uint32_t max_out);

// Builds a complete, valid BRC message wrapping one GRID_CLASS_EVALUATE
// frame - "run this Lua source and report the result", the mechanism the
// desktop editor uses to evaluate live snippets in the module's own Lua VM
// (grid_decode_evaluate_to_ui(), common/src/c/grid_decode.c:609). Unlike
// grid_sim_build_midi_frame(), this class has no existing "build one"
// function anywhere in the codebase to copy - only the receiving side, on a
// real module, ever needs to construct one - so this is composed directly
// from grid_msg_add_frame()/add_segment_char(), the same primitives that
// receiving side's response uses.
//
// GRID_CLASS_EVALUATE is only in grid_decoder_to_ui[], not
// grid_decoder_to_usb[] - inject the result into GRID_SIM_PORT_UI, not
// GRID_SIM_PORT_USB. Writes up to max_out bytes into out and returns how
// many; 0 if lua_code didn't fit.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_build_evaluate_frame(const char* lua_code, uint32_t code_len, char* out, uint32_t max_out);

// Appends a valid footer - EOT + a correctly computed 2-hex-digit checksum +
// '\n' - onto `in`, into `out`. grid_swsr_until_msg_end() (common/src/c/
// grid_swsr.c) - what grid_sim_tick()'s port relay uses to find a message's
// end - specifically looks for that exact EOT+..+\n trailer; anything
// injected without one sits in the port's rx forever, never relayed. Real
// firmware builds every outbound frame the same way (grid_ping_init(),
// common/src/c/grid_port.c: compose with a placeholder checksum, then
// overwrite it via grid_frame_calculate_checksum_packet()+
// grid_str_checksum_set()). Pass a message body with no footer of its own -
// running an already-terminated frame through this again double-appends.
// Writes up to max_out bytes into out (in_len + 4) and returns how many;
// 0 if it didn't fit.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_terminate_frame(const char* in, uint32_t in_len, char* out, uint32_t max_out);

// Feed raw protocol bytes into a port's rx, as if they had just arrived
// over its wire/USB (grid_port_recv_msg(), common/src/c/grid_port.c) -
// handles DCT (handshake) and BRC (broadcast) frames exactly like real
// firmware. A constructed BRC frame's source position header (BRC_SX/
// BRC_SY) determines whether grid_rx_should_handle() (grid_decode.c) treats
// it as this module's own traffic looped back (HANDLE_INTERNAL) or a
// neighbor's (HANDLE_EXTERNAL) - see grid_msg_is_source_internal().
//
// Simplification versus real firmware: this doesn't run the per-target BRC
// position transform (grid_str_transform_brc_params, applied by e.g.
// grid_d51_port_recv_uwsr before calling grid_port_recv_msg) or duplicate-
// broadcast fingerprint suppression - both are about multi-hop relay
// topology, out of scope for a single simulated module.
EMSCRIPTEN_KEEPALIVE void grid_sim_port_inject(uint8_t port, const char* data, uint32_t length);

// Drain (and clear) whatever this module has queued to send out of a
// port's wire/USB since the last call - what a real neighbor or USB host
// would have received. Port 4 (UI) is grid_sim_drain_module_output()'s
// content, exposed here too for uniformity. Empty string if nothing
// pending.
//
// Protocol frames can contain embedded 0x00 bytes (raw parameter data), and
// this returns a NUL-terminated C string - fine for display/logging, but a
// byte at/after the first 0x00 is invisible to it. Use
// grid_sim_port_tx_pending()/grid_sim_port_drain_tx_bytes() below for exact,
// binary-safe access (e.g. before re-injecting a captured frame elsewhere).
EMSCRIPTEN_KEEPALIVE const char* grid_sim_port_drain_tx(uint8_t port);

// How many bytes are currently pending for this port (does not clear them) -
// size grid_sim_port_drain_tx_bytes()'s output buffer with this first.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_port_tx_pending(uint8_t port);

// Binary-safe drain: copies up to max_out pending bytes into out and clears
// exactly that many from the queue (any excess beyond max_out is left
// pending for the next call), returning how many bytes were copied.
EMSCRIPTEN_KEEPALIVE uint32_t grid_sim_port_drain_tx_bytes(uint8_t port, uint8_t* out, uint32_t max_out);

// Is this port currently connected (grid_port_connected(), common/src/c/
// grid_port.c)? USART ports auto-disconnect after
// GRID_PARAMETER_DISCONNECTTIMEOUT_us (500ms) without traffic refreshing
// them, matching real hardware - re-inject a ping to reconnect.
EMSCRIPTEN_KEEPALIVE uint8_t grid_sim_port_is_connected(uint8_t port);

#ifdef __cplusplus
}
#endif

#endif /* WASM_SIM_CORE_H */

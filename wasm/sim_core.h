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

#ifdef __cplusplus
}
#endif

#endif /* WASM_SIM_CORE_H */

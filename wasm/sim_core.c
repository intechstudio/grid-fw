#include "sim_core.h"

#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_decode.h"
#include "grid_font.h"
#include "grid_gui.h"
#include "grid_led.h"
#include "grid_lua.h"
#include "grid_lua_api.h"
#include "grid_module.h"
#include "grid_msg.h"
#include "grid_port.h"
#include "grid_protocol.h"
#include "grid_swsr.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_ui.h"
#include "grid_ui_button.h"
#include "grid_ui_endless.h"
#include "grid_utask.h"

// Defined in platform.c - advances the virtual clock that
// grid_platform_rtc_get_micros() reads. Not part of grid_platform.h because
// it's a simulator-only control, not a hardware hook.
extern void grid_sim_platform_advance_clock(uint32_t dt_ms);

// Per-element-type extra reset hook, checked by grid_ui_element_reset()
// (common/src/c/grid_ui.c). Every target defines this itself - see
// d51n20a/grid/d51/grid_d51.c - and none of them populate it yet, so an
// all-NULL table is the correct, faithful default here too.
const grid_ui_element_state_reset_t grid_ui_element_state_resets[GRID_PARAMETER_ELEMENT_COUNT] = {};
const grid_ui_element_state_any_t grid_ui_element_state_anys[GRID_PARAMETER_ELEMENT_COUNT] = {};

static uint8_t sim_lcd_framebuffer[GRID_SIM_LCD_WIDTH * GRID_SIM_LCD_HEIGHT * GRID_SIM_LCD_BYTES_PER_PIXEL];

// Real firmware never actually calls grid_esp32_lcd_task()'s draw-trigger
// equivalent for anything but the physical LCD driver - see
// esp32s3/components/grid_esp32_lcd/grid_esp32_lcd.c:404-407, which fires
// the LCD element's DRAW event directly (grid_ui_process_single(), not the
// TRIG/grid_ui_process_triggered() path other elements use) on a fixed
// GRID_PARAMETER_DRAWTRIGGER_us timer, skipping a cycle if the previous
// draw's command queue hasn't drained yet. Replicated here since nothing
// else would ever trigger the LCD's DRAW event at all.
static struct grid_utask_timer sim_lcd_draw_timer;

// grid_transport_send_msg_to_all() (used by every triggered element event,
// grid_ui_clear_triggered(), common/src/c/grid_ui.c) writes straight into
// port 4's ("UI") rx buffer - see sim_decode_ui_port() below, which is what
// actually decodes that and drives self.eventrx_cb() on elements that
// define one (e.g. VSN1's LCD - see grid_ui_lcd.h's
// GRID_ACTIONSTRING_LCD_INIT). grid_sim_drain_module_output() has its own
// copy appended here rather than reading port 4's rx directly, since that
// buffer is a single-reader queue that sim_decode_ui_port() also consumes
// every tick - both need to see everything sent.
static char sim_module_output_buf[2048];
static size_t sim_module_output_len = 0;

static void sim_append_module_output(const char* data, size_t len) {
  size_t avail = sizeof(sim_module_output_buf) - sim_module_output_len - 1;
  if (len > avail) {
    len = avail;
  }
  memcpy(sim_module_output_buf + sim_module_output_len, data, len);
  sim_module_output_len += len;
}

// Real firmware doesn't wire this up on any target yet (see the analysis
// that led here) - every module always broadcasts EVENTVIEW frames for its
// own events, but nothing anywhere calls grid_port_decode_msg() to actually
// decode them back into Lua. VSN1's default config only makes sense with
// this in place: its LCD screen is designed to show the name/value of the
// last-interacted element (this module's own, or a neighbor's).
static void sim_decode_ui_port(void) {

  struct grid_swsr_t* rx = grid_port_get_rx(&grid_transport_state.ports[4]);

  struct grid_msg msg;
  while (grid_msg_from_swsr(&msg, rx)) {
    sim_append_module_output(msg.data, msg.length);
    grid_port_decode_msg(grid_decoder_to_ui_reference, &msg);
  }
}

static void sim_lcd_draw_tick(void) {

  if (!grid_utask_timer_elapsed(&sim_lcd_draw_timer)) {
    return;
  }

  // Previous frame's draw commands haven't drained yet - skip this cycle
  // rather than piling more commands on top, matching real firmware.
  if (grid_swsr_size(&grid_gui_states[0].swsr)) {
    return;
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, GRID_SIM_LCD_ELEMENT);
  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_DRAW);
  grid_ui_process_single(&grid_ui_state, ele, eve);
}

// grid_ui_button_store_input() only starts producing button/endless events
// once it has observed both a low and a high raw value (see
// grid_ui_button_state_range_valid, common/src/c/grid_ui_button.c) - real
// hardware gets there from ADC noise within the first few samples. Feed
// both extremes once up front so every element is calibrated and quiet
// (released) before any real input arrives.
static void sim_prime_button(struct grid_ui_button_state* state) {
  grid_ui_button_store_input(state, 0);
  grid_ui_button_store_input(state, GRID_ADC_MAX);
}

// Calibration (grid_ui_*_state_init) has to run before grid_lua_start_vm()/
// the bulk page load below, since grid_module_vsnl_ui_init() only allocates
// each element's primary_state - it doesn't set up its threshold/hysteresis.
static void sim_init_element_states(void) {

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    switch (ele->type) {

    case GRID_PARAMETER_ELEMENT_BUTTON:
      grid_ui_button_state_init(grid_ui_button_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
      break;

    case GRID_PARAMETER_ELEMENT_ENDLESS:
      grid_ui_endless_state_init(grid_ui_endless_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
      break;

    default:
      break;
    }
  }
}

// Priming (see sim_prime_button()) writes through ele->template_parameter_list,
// which is only assigned once the page-0 bulk load below has run (it's what
// creates each element's template buffer - see grid_ui_encoder_test.c for
// the same dependency at the unit-test level). Must run after that load, or
// grid_ui_button_store_input() dereferences a template_parameter_list that
// doesn't exist yet.
static void sim_prime_elements(void) {

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    switch (ele->type) {

    case GRID_PARAMETER_ELEMENT_BUTTON:
      sim_prime_button(grid_ui_button_get_state(ele));
      break;

    case GRID_PARAMETER_ELEMENT_ENDLESS:
      sim_prime_button(&grid_ui_endless_get_state(ele)->button);
      break;

    default:
      break;
    }
  }
}

void grid_sim_init(void) {

  // Calibration persistence model - VSN1L has no potmeter elements, so this
  // is only here to satisfy grid_config_init()'s requirement of a non-NULL
  // cal model; it's otherwise unused.
  grid_cal_init(&grid_cal_state, 16, GRID_AIN_INTERNAL_RESOLUTION);
  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_msg_model_init(&grid_msg_state);
  grid_sys_init(&grid_sys_state);

  // grid_sys_init() reads this from grid_platform_get_hwcfg(), which this
  // simulator stubs to 0 (no real hardware ID straps to read) - real
  // firmware branches on grid_hwcfg_module_is_*() at boot instead of
  // calling grid_module_vsnl_ui_init() directly like grid_sim_init() does,
  // so it never needed this override. But hwfcg also drives feature checks
  // sprinkled through Lua and C alike (ghaslcd(), grid_hwcfg_module_is_vsnl(),
  // is_rev_h(), encoder_dir(), ...) that this simulator's Lua config still
  // exercises - e.g. init.lua's "if ghaslcd() then" is what enables
  // listening for EVENTVIEW messages at all. Set it directly to VSN1L so
  // those all resolve the way they would on the real module being simulated.
  grid_sys_state.hwfcg = GRID_MODULE_VSN1L_RevA;

  // grid_sys_init() only zeroes the *active* bank color - grid_sys_set_bank()
  // is what actually resolves it from the bank palette (matches real
  // firmware's boot sequence, e.g. esp32s3/main/grid_esp32s3.c). Without
  // this, glr()/glg()/glb() (common/src/lua/simplecolor.lua's bank-color
  // getters, used by every element's default LED color script) all read
  // back (0,0,0), so every LED stays black regardless of element state.
  grid_sys_set_bank(&grid_sys_state, 0);

  // Ports are allocated so grid_lua_api's grid_transport_send_msg_to_all()
  // (used by grid_send()) has somewhere to write - this is a single-module
  // simulation, so nothing drains them onto real UART/USB.
  const int PORT_COUNT = 6;
  grid_transport_malloc(&grid_transport_state, PORT_COUNT);
  grid_port_init(&grid_transport_state.ports[0], GRID_PORT_USART, GRID_PORT_NORTH);
  grid_port_init(&grid_transport_state.ports[1], GRID_PORT_USART, GRID_PORT_EAST);
  grid_port_init(&grid_transport_state.ports[2], GRID_PORT_USART, GRID_PORT_SOUTH);
  grid_port_init(&grid_transport_state.ports[3], GRID_PORT_USART, GRID_PORT_WEST);
  grid_port_init(&grid_transport_state.ports[4], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[5], GRID_PORT_USB, 0);

  // The LCD element's default init script draws immediately (see
  // GRID_ACTIONSTRING_LCD_INIT, common/src/c/grid_ui_lcd.h), so the screen
  // it draws to must exist before the module/Lua init below runs.
  grid_font_init(&grid_font_state);
  grid_gui_init(&grid_gui_states[0], NULL, sim_lcd_framebuffer, sizeof(sim_lcd_framebuffer), GRID_SIM_LCD_WIDTH, GRID_SIM_LCD_HEIGHT);
  grid_gui_clear(&grid_gui_states[0], grid_gui_color_from_rgb(0, 0, 0));
  grid_gui_swap_set(&grid_gui_states[0], true);

  // Builds the element list (buttons/endless/lcd/system) and sets
  // grid_ui_state.lua_ui_init_callback = grid_lua_ui_init.
  grid_module_vsnl_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  sim_init_element_states();

  // grid_lua_init() only configures the allocator - it does not open the
  // Lua state. grid_ui_bulk_page_load (triggered below) is what actually
  // does that, via its own grid_lua_stop_vm()+grid_lua_start_vm() call
  // (common/src/c/grid_ui.c) - matching real firmware, which never calls
  // grid_lua_start_vm() itself outside of that same page-load path.
  grid_lua_init(&grid_lua_state, NULL, NULL);

  // Loads page 0 (falls back to every element's default script, since this
  // simulator has no persisted per-page scripts) and runs each element's
  // INIT event - this is what actually executes GRID_ACTIONSTRING_*_INIT,
  // starts the Lua VM (registering every element's Lua object via
  // grid_ui_state.lua_ui_init_callback), and allocates each element's
  // template_parameter_list, so priming (below) has to come after it.
  grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_page_load, 0, 0, NULL);
  grid_ui_bulk_flush(&grid_ui_state);

  sim_prime_elements();

  sim_lcd_draw_timer = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = GRID_PARAMETER_DRAWTRIGGER_us,
  };
  sim_module_output_len = 0;
}

int grid_sim_load_config(const char* toml) {

  size_t len = strlen(toml);
  char* buf = (char*)malloc(len + 1);
  memcpy(buf, toml, len + 1);

  int result = grid_config_parse(&grid_config_state, buf);

  free(buf);

  return result;
}

void grid_sim_tick(uint32_t dt_ms) {

  grid_sim_platform_advance_clock(dt_ms);

  for (uint32_t i = 0; i < dt_ms; ++i) {
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
  }

  grid_led_tick(&grid_led_state);
  grid_led_render_framebuffer(&grid_led_state);

  grid_ui_bulk_process(&grid_ui_state);
  grid_ui_process_triggered(&grid_ui_state);

  // Consume whatever grid_ui_process_triggered() just broadcast (EVENTVIEW
  // frames included) before triggering this frame's LCD redraw, so a
  // newly-updated self.eventrx_cb value can show up as early as this tick.
  sim_decode_ui_port();

  sim_lcd_draw_tick();

  while (grid_swsr_size(&grid_gui_states[0].swsr)) {
    grid_gui_queue_step(&grid_gui_states[0]);
  }
}

void grid_sim_input_button(uint8_t element, uint8_t pressed) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);
  if (!ele || ele->type != GRID_PARAMETER_ELEMENT_BUTTON) {
    return;
  }

  grid_ui_button_store_input(grid_ui_button_get_state(ele), pressed ? 0 : GRID_ADC_MAX);
}

void grid_sim_input_endless_rotate(uint8_t element, int16_t delta) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);
  if (!ele || ele->type != GRID_PARAMETER_ELEMENT_ENDLESS) {
    return;
  }

  struct grid_ui_endless_state* state = grid_ui_endless_get_state(ele);

  // grid_ui_endless_update_trigger()'s velocity math (common/src/c/
  // grid_ui_endless.c) divides by elapsed real time since the *previous*
  // rotate call, so how large a raw delta needs to be to actually cross its
  // trigger threshold grows with the gap between calls - not just a fixed
  // "182 out of 65536 is noise" floor. Empirically, 4200 reliably triggers
  // across a wide range of gaps (idle for a while, then one rotate call, up
  // through a slow ~200ms-per-step drag); smaller values work for a quick
  // drag but silently do nothing for a slower one. Clamped to int16_t range
  // since that's what the real decoder produces.
  enum { GRID_SIM_ENDLESS_RAW_PER_STEP = 5000 };
  int32_t raw_delta = (int32_t)delta * GRID_SIM_ENDLESS_RAW_PER_STEP;
  if (raw_delta > INT16_MAX) {
    raw_delta = INT16_MAX;
  } else if (raw_delta < INT16_MIN) {
    raw_delta = INT16_MIN;
  }

  grid_ui_endless_update_trigger(ele, 0, (int16_t)raw_delta, &state->delta_vel_frac, &state->encoder_last_real_time);
}

void grid_sim_input_endless_button(uint8_t element, uint8_t pressed) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);
  if (!ele || ele->type != GRID_PARAMETER_ELEMENT_ENDLESS) {
    return;
  }

  struct grid_ui_endless_state* state = grid_ui_endless_get_state(ele);
  grid_ui_button_store_input(&state->button, pressed ? 0 : GRID_ADC_MAX);
}

uint8_t grid_sim_get_element_type(uint8_t element) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);
  return ele ? ele->type : 0xff;
}

void grid_sim_get_led_rgb(uint8_t num, uint8_t* r, uint8_t* g, uint8_t* b) {

  if (num >= grid_led_get_led_count(&grid_led_state)) {
    *r = *g = *b = 0;
    return;
  }

  // led_frame_buffer is stored GRB per LED (see grid_led_render_framebuffer_one,
  // common/src/c/grid_led.c) - reorder to plain RGB for callers.
  uint8_t* fb = grid_led_get_framebuffer_pointer(&grid_led_state);
  *g = fb[num * 3 + 0];
  *r = fb[num * 3 + 1];
  *b = fb[num * 3 + 2];
}

uint8_t grid_sim_get_led_count(void) { return (uint8_t)grid_led_get_led_count(&grid_led_state); }

uint32_t grid_sim_get_led_rgb_packed(uint8_t num) {
  uint8_t r, g, b;
  grid_sim_get_led_rgb(num, &r, &g, &b);
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t grid_sim_get_element_led_rgb_packed(uint8_t element) {
  uint8_t led;
  if (grid_sim_get_element_led_indices(element, &led, 1) == 0) {
    return 0;
  }
  return grid_sim_get_led_rgb_packed(led);
}

uint8_t grid_sim_get_element_led_indices(uint8_t element, uint8_t* out, uint8_t max_out) {

  uint8_t length = 0;
  uint8_t* indices = grid_led_lookup_get(&grid_led_state, element, &length);

  if (!indices) {
    return 0;
  }

  uint8_t n = length < max_out ? length : max_out;
  memcpy(out, indices, n);

  return n;
}

uint8_t* grid_sim_get_lcd_framebuffer(void) { return sim_lcd_framebuffer; }

const char* grid_sim_drain_module_output(void) {

  static char out[sizeof(sim_module_output_buf)];

  // See sim_module_output_buf's comment above: this is a copy taken as
  // sim_decode_ui_port() (called every grid_sim_tick()) consumes port 4's
  // real rx queue, not a read of that queue itself.
  memcpy(out, sim_module_output_buf, sim_module_output_len);
  out[sim_module_output_len] = '\0';
  sim_module_output_len = 0;

  return out;
}

const char* grid_sim_drain_stdo(void) {

  static char out[GRID_LUA_STDO_LENGTH];
  memcpy(out, grid_lua_state.stdo, GRID_LUA_STDO_LENGTH);
  grid_lua_clear_stdo(&grid_lua_state);
  return out;
}

const char* grid_sim_drain_stde(void) {

  static char out[GRID_LUA_STDE_LENGTH];
  memcpy(out, grid_lua_state.stde, GRID_LUA_STDE_LENGTH);
  grid_lua_clear_stde(&grid_lua_state);
  return out;
}

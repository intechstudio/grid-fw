#ifndef GRID_LUA_API_H
#define GRID_LUA_API_H

#include <stdint.h>
#include <stdlib.h>

#include <string.h>

#include "grid_led.h"
#include "grid_lua.h"
#include "grid_msg.h"
#include "grid_protocol.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_ui.h"

extern struct grid_led_model grid_led_state;

extern void grid_platform_printf(char const* fmt, ...);
extern void grid_platform_delay_ms(uint32_t delay_milliseconds);

/* ==================== LUA C API REGISTERED FUNCTIONS  ====================*/

/*static*/ int l_my_print(lua_State* L);

/*static*/ int l_grid_list_dir(lua_State* L);

/*static*/ int l_grid_websocket_send(lua_State* L);
/*static*/ int l_grid_package_send(lua_State* L);
/*static*/ int l_grid_immediate_send(lua_State* L);
/*static*/ int l_grid_elementname_send(lua_State* L);
/*static*/ int l_grid_string_get(lua_State* L);

/*static*/ int l_grid_usb_keyboard_send(lua_State* L);
/*static*/ int l_grid_mousemove_send(lua_State* L);
/*static*/ int l_grid_mousebutton_send(lua_State* L);

/*static*/ int l_grid_send(lua_State* L);

// ==== MIDI ====

/*static*/ int l_grid_midirx_enabled(lua_State* L);
/*static*/ int l_grid_midirx_sync(lua_State* L);

/*static*/ int l_grid_midi_send(lua_State* L);
/*static*/ int l_grid_midi_sysex_send(lua_State* L);

// ==== LED ====

// /*static*/ int l_grid_led_layer_phase(lua_State* L);  depends on grid_ui.c
/*static*/ int l_grid_led_layer_min(lua_State* L);
/*static*/ int l_grid_led_layer_mid(lua_State* L);
/*static*/ int l_grid_led_layer_max(lua_State* L);

/*static*/ int l_grid_led_layer_color(lua_State* L);
/*static*/ int l_grid_led_layer_frequency(lua_State* L);
/*static*/ int l_grid_led_layer_shape(lua_State* L);

/*static*/ int l_grid_led_layer_timeout(lua_State* L);
// /*static*/ int l_grid_led_layer_pfs(lua_State* L);   depends on grid_ui.c

/*static*/ int l_led_default_red(lua_State* L);
/*static*/ int l_led_default_green(lua_State* L);
/*static*/ int l_led_default_blue(lua_State* L);

/*static*/ int l_grid_version_major(lua_State* L);
/*static*/ int l_grid_version_minor(lua_State* L);
/*static*/ int l_grid_version_patch(lua_State* L);
/*static*/ int l_grid_hwcfg(lua_State* L);
/*static*/ int l_grid_random8(lua_State* L);
/*static*/ int l_grid_position_x(lua_State* L);
/*static*/ int l_grid_position_y(lua_State* L);
/*static*/ int l_grid_rotation(lua_State* L);

/*static*/ int l_grid_led_layer_phase(lua_State* L);
/*static*/ int l_grid_led_layer_pfs(lua_State* L);
/*static*/ int l_grid_template_variable(lua_State* L);
/*static*/ int l_grid_page_next(lua_State* L);
/*static*/ int l_grid_page_prev(lua_State* L);
/*static*/ int l_grid_page_curr(lua_State* L);
/*static*/ int l_grid_page_load(lua_State* L);
/*static*/ int l_grid_timer_start(lua_State* L);
/*static*/ int l_grid_timer_stop(lua_State* L);
/*static*/ int l_grid_event_trigger(lua_State* L);

/*static*/ int l_grid_element_count(lua_State* L);

/*static*/ int l_grid_potmeter_calibration_get(lua_State* L);
/*static*/ int l_grid_potmeter_calibration_set(lua_State* L);

extern const struct luaL_Reg* grid_lua_api_generic_lib_reference;

#endif /* GRID_LUA_API_H */

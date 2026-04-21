#ifndef GRID_LUA_API_H
#define GRID_LUA_API_H

#include <stdint.h>

#include "grid_lua.h"

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
/*static*/ int l_grid_elementname_set(lua_State* L);
/*static*/ int l_grid_elementname_get(lua_State* L);
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

/*static*/ int l_grid_led_address_get(lua_State* L);

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
/*static*/ int l_grid_potmeter_detent_set(lua_State* L);
/*static*/ int l_grid_button_calibration_get(lua_State* L);
/*static*/ int l_grid_button_calibration_set(lua_State* L);

/*static*/ int l_grid_lcd_set_backlight(lua_State* L);

int GRID_LUA_FNC_GTV_NAME(0)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(1)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(2)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(3)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(4)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(5)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(6)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(7)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(8)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(9)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(10)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(11)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(12)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(13)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(14)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(15)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(16)(lua_State*);
int GRID_LUA_FNC_GTV_NAME(17)(lua_State*);

int GRID_LUA_FNC_META_NAME(gtt)(lua_State*);
int GRID_LUA_FNC_META_NAME(gtp)(lua_State*);
int GRID_LUA_FNC_META_NAME(get)(lua_State*);
int GRID_LUA_FNC_META_NAME(gsen)(lua_State*);
int GRID_LUA_FNC_META_NAME(ggen)(lua_State*);

int GRID_LUA_FNC_DRAW_NAME(ldsw)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldpx)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldl)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldr)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldrf)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldrr)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldrrf)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldpo)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldpof)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldt)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldft)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldaf)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(ldd)(lua_State*);
int GRID_LUA_FNC_DRAW_NAME(lgrt)(lua_State*);

extern const struct luaL_Reg* grid_lua_api_generic_lib_reference;

#endif /* GRID_LUA_API_H */

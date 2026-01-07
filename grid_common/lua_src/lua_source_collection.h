#ifndef LUA_SOURCE_COLLECTION_H_INCLUDED
#define LUA_SOURCE_COLLECTION_H_INCLUDED

#include "decode.h"
#define GRID_LUA_FNC_G_DECODE_source grid_lua_src_decode_lua
#define GRID_LUA_FNC_G_DECODE_short "gdec"
#define GRID_LUA_FNC_G_DECODE_human "decode"

#include "lookup.h"
#define GRID_LUA_FNC_G_LOOKUP_source grid_lua_src_lookup_lua
#define GRID_LUA_FNC_G_LOOKUP_short "glut"
#define GRID_LUA_FNC_G_LOOKUP_human "lookup"

#include "elementname.h"
#define GRID_LUA_FNC_G_ELEMENTNAME_source grid_lua_src_elementname_lua
#define GRID_LUA_FNC_G_ELEMENTNAME_short "gen"
#define GRID_LUA_FNC_G_ELEMENTNAME_human "element_name"

#include "eventfname.h"
#define GRID_LUA_FNC_G_EVENTFNAME_source grid_lua_src_eventfname_lua

#include "limit.h"
#define GRID_LUA_FNC_G_LIMIT_source grid_lua_src_limit_lua
#define GRID_LUA_FNC_G_LIMIT_short "glim"
#define GRID_LUA_FNC_G_LIMIT_human "limit"

#include "mapsat.h"
#define GRID_LUA_FNC_G_MAPSAT_source grid_lua_src_mapsat_lua
#define GRID_LUA_FNC_G_MAPSAT_short "gmaps"
#define GRID_LUA_FNC_G_MAPSAT_human "map_saturate"

#include "sign.h"
#define GRID_LUA_FNC_G_SIGN_source grid_lua_src_sign_lua
#define GRID_LUA_FNC_G_SIGN_short "sgn"
#define GRID_LUA_FNC_G_SIGN_human "sign"

#include "segmentcalc.h"
#define GRID_LUA_FNC_G_SEGCALC_source grid_lua_src_segmentcalc_lua
#define GRID_LUA_FNC_G_SEGCALC_short "gsc"
#define GRID_LUA_FNC_G_SEGCALC_human "segment_calculate"

#include "toml.h"
#define GRID_LUA_FNC_G_TOML_source grid_lua_src_toml_lua

#include "simplecolor.h"
#define GRID_LUA_FNC_G_SIMPLECOLOR_source grid_lua_src_simplecolor_lua

#define GRID_LUA_FNC_G_COLOR_CURVE_short "color_curve"
#define GRID_LUA_FNC_G_COLOR_CURVE_human "color_curve"
#define GRID_LUA_FNC_G_COLOR_CURVE_usage "color_curve(table color_array) Calculates three-point intensity response curve (min, mid, max) from color array. Processes -1 values as current LED colors. Returns three color tables. User overridable."

#define GRID_LUA_FNC_G_COLOR_AUTO_LAYER_short "color_auto_layer"
#define GRID_LUA_FNC_G_COLOR_AUTO_LAYER_human "color_auto_layer"
#define GRID_LUA_FNC_G_COLOR_AUTO_LAYER_usage "color_auto_layer(element self) Calculates LED layer based on current event type. Returns 1 for button/potentiometer events, 2 for encoder events, nil for others. User overridable."

#define GRID_LUA_FNC_G_COLOR_AUTO_VALUE_short "color_auto_value"
#define GRID_LUA_FNC_G_COLOR_AUTO_VALUE_human "color_auto_value"
#define GRID_LUA_FNC_G_COLOR_AUTO_VALUE_usage "color_auto_value(element self, int segment_index) Calculates LED intensity value based on element type and state. For endless encoders, returns segment-specific value. For other elements, returns mapped value 0-255. User overridable."

#include "simplemidi.h"
#define GRID_LUA_FNC_G_SIMPLEMIDI_source grid_lua_src_simplemidi_lua

#define GRID_LUA_FNC_G_MIDI_AUTO_CH_short "midi_auto_ch"
#define GRID_LUA_FNC_G_MIDI_AUTO_CH_human "midi_auto_ch"
#define GRID_LUA_FNC_G_MIDI_AUTO_CH_usage "midi_auto_ch(element self) Calculates default MIDI channel from grid position. Returns channel 0-15 based on module Y position and page number. User overridable."

#define GRID_LUA_FNC_G_MIDI_AUTO_CMD_short "midi_auto_cmd"
#define GRID_LUA_FNC_G_MIDI_AUTO_CMD_human "midi_auto_cmd"
#define GRID_LUA_FNC_G_MIDI_AUTO_CMD_usage "midi_auto_cmd(element self) Calculates default MIDI command type. Returns 144 (Note On) for button events, 176 (Control Change) for others. User overridable."

#define GRID_LUA_FNC_G_MIDI_AUTO_P1_short "midi_auto_p1"
#define GRID_LUA_FNC_G_MIDI_AUTO_P1_human "midi_auto_p1"
#define GRID_LUA_FNC_G_MIDI_AUTO_P1_usage "midi_auto_p1(element self) Calculates default MIDI parameter 1 (note/CC number) from grid position. Returns 0-127 based on module X position and element index. User overridable."

#define GRID_LUA_FNC_G_MIDI_AUTO_P2_short "midi_auto_p2"
#define GRID_LUA_FNC_G_MIDI_AUTO_P2_human "midi_auto_p2"
#define GRID_LUA_FNC_G_MIDI_AUTO_P2_usage "midi_auto_p2(element self) Calculates default MIDI parameter 2 (value) from element state. Returns value based on event type (button, encoder, potentiometer). User overridable."

#include "autovalue.h"
#define GRID_LUA_FNC_G_AUTOVALUE_source grid_lua_src_autovalue_lua

#endif

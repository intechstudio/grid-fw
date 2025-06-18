#ifndef LUA_SOURCE_COLLECTION_H_INCLUDED
#define LUA_SOURCE_COLLECTION_H_INCLUDED

#include "timer.h"
#define GRID_LUA_FNC_G_TIMER_source grid_lua_src_timer_lua

#include "lookup.h"
#define GRID_LUA_FNC_G_LOOKUP_source grid_lua_src_lookup_lua
#define GRID_LUA_FNC_G_LOOKUP_short "glut"
#define GRID_LUA_FNC_G_LOOKUP_human "lookup"

#include "elementname.h"
#define GRID_LUA_FNC_G_ELEMENTNAME_source grid_lua_src_elementname_lua
#define GRID_LUA_FNC_G_ELEMENTNAME_short "gen"
#define GRID_LUA_FNC_G_ELEMENTNAME_human "element_name"

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

#endif

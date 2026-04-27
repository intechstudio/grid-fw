#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "grid_platform.h"

static int dirent_list(lua_State* L) {

	const char* path = luaL_optstring(L, 1, ".");

	void* dir = grid_platform_opendir(path);
	if (!dir) {
		return luaL_argerror(L, 1, "failed to open directory");
	}

	lua_newtable(L);

	int i = 0;
	void* info;
	while ((info = grid_platform_readdir(dir))) {

		lua_newtable(L);

		lua_pushstring(L, grid_platform_file_info_name(info));
		lua_rawseti(L, -2, 1);
		lua_pushinteger(L, grid_platform_file_info_type(info));
		lua_rawseti(L, -2, 2);

		lua_rawseti(L, -2, ++i);
	}

	if (grid_platform_closedir(dir) != 0) {
		return luaL_error(L, "failed to close directory");
	}

	return 1;
}

static int dirent_mkdir(lua_State* L) {

	const char* path = luaL_checkstring(L, 1);

	lua_pushboolean(L, grid_platform_mkdir(path) == 0);

	return 1;
}

static const luaL_Reg direntlib[] = {
	{"list", dirent_list},
	{"mkdir", dirent_mkdir},
	{NULL, NULL},
};

LUAMOD_API int luaopen_dirent(lua_State* L) {

	luaL_newlib(L, direntlib);

	return 1;
}

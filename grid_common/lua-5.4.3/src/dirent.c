#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "grid_platform.h"

static int dirent_dir(lua_State* L) {

	const char* path = luaL_optstring(L, 1, ".");

	void* dir = grid_platform_opendir(path);
	if (!dir) {
		return luaL_argerror(L, 1, "failed to open directory");
	}

	lua_newtable(L);

	int i = 0;
	void* dirent = NULL;
	while ((dirent = grid_platform_readdir(dir))) {
		lua_pushstring(L, dirent);
		lua_rawseti(L, -2, ++i);
	}

	if (grid_platform_closedir(dir) != 0) {
		return luaL_error(L, "failed to close directory");
	}

	return 1;
}

static const luaL_Reg direntlib[] = {
	{"dir", dirent_dir},
	{NULL, NULL},
};

LUAMOD_API int luaopen_dirent(lua_State* L) {

	luaL_newlib(L, direntlib);

	return 1;
}

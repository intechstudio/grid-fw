#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "grid_platform.h"
#include "../../littlefs/lfs.h"

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

static int dirent_list(lua_State* L) {

	const char* path = luaL_optstring(L, 1, ".");

	void* dir = grid_platform_opendir(path);
	if (!dir) {
		return luaL_argerror(L, 1, "failed to open directory");
	}

	lua_newtable(L);

	int i = 0;
	const char* name = NULL;
	while ((name = grid_platform_readdir(dir))) {
		int type = grid_platform_readdir_type();
		lua_newtable(L);
		lua_pushstring(L, name);
		lua_setfield(L, -2, "name");
		lua_pushstring(L, type == LFS_TYPE_DIR ? "dir" : "file");
		lua_setfield(L, -2, "type");
		lua_rawseti(L, -2, ++i);
	}

	if (grid_platform_closedir(dir) != 0) {
		return luaL_error(L, "failed to close directory");
	}

	return 1;
}


static int dirent_mkdir(lua_State* L) {

	const char* path = luaL_checkstring(L, 1);

	int result = grid_platform_make_directory(path);
	if (result != 0) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed to create directory");
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

static const luaL_Reg direntlib[] = {
	{"dir",   dirent_dir},
	{"list",  dirent_list},
	{"mkdir", dirent_mkdir},
	{NULL, NULL},
};

LUAMOD_API int luaopen_dirent(lua_State* L) {

	luaL_newlib(L, direntlib);

	return 1;
}

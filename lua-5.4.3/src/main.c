#include <stdio.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


int main(){

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);



	int loopc = 0;

	char * code = "return (3+4+5)";
	
	//lua_pop(L, lua_gettop(L));

	for (int i=0; i<150; i = 0){

loopc++;
		int top = lua_gettop(L);

			
		if (luaL_loadstring(L, code) == LUA_OK) {

			if (lua_pcall(L, 0, -1, 0) == LUA_OK){

				// If it was executed successfuly we 
				// remove the code from the stack

				int top2 = lua_gettop(L);
				if (lua_isnumber(L, -1)){

					int res1 = lua_tonumber(L, -1);
					printf("%d: LUAOK %s -> %d (Top: %d %d)\r\n",loopc, code, res1, top, top2);
				}
				else {
					printf("Notnumber\r\n");
				}

			}
			else {

				
			}

			lua_pop(L, lua_gettop(L));

		}
		else{

			printf("LUA not OK\r\n");
		}
		

	}

	printf("DONE\r\n");
	return 0;
}

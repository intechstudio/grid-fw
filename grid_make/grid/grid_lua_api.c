/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_lua_api.h"


int _gettimeofday(){return 0;} 
int _open(){while(1);} 
int _times(){while(1);} 
int _unlink(){while(1);} 
int _link(){while(1);} 


static int grid_lua_panic(lua_State *L) {

    while(1){

        printf("LUA PANIC\r\n");
        delay_ms(1000);
    }
}


static int l_my_print(lua_State* L) {

    int nargs = lua_gettop(L);
    printf("LUA PRINT: ");
    for (int i=1; i <= nargs; ++i) {

        if (lua_type(L, i) == LUA_TSTRING){

		    printf(" str: %s ", lua_tostring(L, i));
        }
        else if (lua_type(L, i) == LUA_TNUMBER){

            lua_Number lnum = lua_tonumber(L, i);
            lua_Integer lint;
            lua_numbertointeger(lnum, &lint);
            //int32_t num = lua_tonumber

		    printf(" num: %d ", (int)lnum);
        }
    }
    printf("\r\n");

    return 0;
}

static int l_grid_send(lua_State* L) {

    int nargs = lua_gettop(L);

    char start_of_text[2] = {GRID_CONST_STX, 0};
    
    strcat(grid_lua_state.stdo, start_of_text);

    for (int i=1; i <= nargs; ++i) {
        strcat(grid_lua_state.stdo, lua_tostring(L, i));
    }

    char end_of_text[2] =   {GRID_CONST_ETX, 0};

    strcat(grid_lua_state.stdo, end_of_text);

    return 0;
}


static const struct luaL_Reg printlib [] = {
  {"print", l_my_print},
  {"grid_send", l_grid_send},
  {NULL, NULL} /* end of array */
};


uint8_t grid_lua_init(struct grid_lua_model* mod){


    mod->stdo_len = GRID_LUA_STDO_LENGTH;
    mod->stdi_len = GRID_LUA_STDI_LENGTH;

    grid_lua_clear_stdo(mod);
    grid_lua_clear_stdi(mod);

}

uint8_t grid_lua_deinit(struct grid_lua_model* mod){


}

uint8_t grid_lua_debug_memory_stats(struct grid_lua_model* mod, char* message){

    uint32_t memusage = lua_gc(grid_lua_state.L, LUA_GCCOUNT)*1024 + lua_gc(grid_lua_state.L, LUA_GCCOUNTB);
    //printf("LUA mem usage: %d(%s)\r\n", memusage, message);

}


uint8_t grid_lua_start_vm(struct grid_lua_model* mod){

	mod->L = luaL_newstate();

    lua_atpanic(mod->L, &grid_lua_panic);

    grid_lua_debug_memory_stats(mod, "Init");


    luaL_openlibs(mod->L);


    grid_lua_debug_memory_stats(mod, "Openlibs");

    lua_getglobal(mod->L, "_G");
	luaL_setfuncs(mod->L, printlib, 0);
	lua_pop(mod->L, 1);
    grid_lua_debug_memory_stats(mod, "Printlib");


    grid_lua_dostring(mod, "p2x = function(num) local a local b  if num%16 < 10 then a = string.char(48+num%16) else a = string.char(97+num%16-10) end if num//16 < 10 then b = string.char(48+num//16) else b = string.char(97+num//16-10) end return b .. a end");
    grid_lua_debug_memory_stats(mod, "P2X");

    grid_lua_dostring(mod, "grid_send_midi = function(ch, cmd, p1, p2) grid_send('000e', p2x(ch), p2x(cmd), p2x(p1), p2x(p2)) end");
    grid_lua_debug_memory_stats(mod, "grid_send");

}

uint8_t grid_lua_stop_vm(struct grid_lua_model* mod){

    lua_close(mod->L);
}



uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code){

    if (luaL_loadstring(mod->L, code) == LUA_OK){

        if (( lua_pcall(mod->L, 0, LUA_MULTRET, 0)) == LUA_OK) {
            // If it was executed successfuly we 
            // remove the code from the stack

        }
        else{
            printf("LUA not OK\r\n");
        }

        lua_pop(mod->L, lua_gettop(mod->L));
    
    }
    else{
        printf("LUA not OK\r\n");
    }

}


void grid_lua_clear_stdi(struct grid_lua_model* mod){

    for (uint32_t i=0; i<mod->stdi_len; i++){
        mod->stdi[i] = 0;
    }

}

void grid_lua_clear_stdo(struct grid_lua_model* mod){


    for (uint32_t i=0; i<mod->stdo_len; i++){
        mod->stdo[i] = 0;
    }


}





/*
 * grid_lua.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_lua_api.h"


struct grid_lua_model grid_lua_state;

void grid_lua_init(struct grid_lua_model* mod){


    mod->stdo_len = GRID_LUA_STDO_LENGTH;
    mod->stdi_len = GRID_LUA_STDI_LENGTH;
    mod->stde_len = GRID_LUA_STDE_LENGTH;

    grid_lua_clear_stdo(mod);
    grid_lua_clear_stdi(mod);
    grid_lua_clear_stde(mod);

    mod->dostring_count = 0;

}

void grid_lua_deinit(struct grid_lua_model* mod){


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



void grid_lua_clear_stde(struct grid_lua_model* mod){


    for (uint32_t i=0; i<mod->stde_len; i++){
        mod->stde[i] = 0;
    }


}


char* grid_lua_get_output_string(struct grid_lua_model* mod){
    return mod->stdo;
}

char* grid_lua_get_error_string(struct grid_lua_model* mod){
    return mod->stde;
}


uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code){

    mod->dostring_count++;

    uint32_t is_ok = 1;

    if (luaL_loadstring(mod->L, code) == LUA_OK){

        if (( lua_pcall(mod->L, 0, LUA_MULTRET, 0)) == LUA_OK) {
            // If it was executed successfuly we 
            // remove the code from the stack

        }
        else{
            //grid_platform_printf("LUA not OK: %s \r\n", code);
            //grid_port_debug_printf("LUA not OK");
            is_ok = 0;
        }

        lua_pop(mod->L, lua_gettop(mod->L));
    
    }
    else{
        //grid_platform_printf("LUA not OK:  %s\r\n", code);
        //grid_port_debug_printf("LUA not OK");
        is_ok = 0;
    }


    grid_lua_gc_try_collect(mod);

    return is_ok;

}




void grid_lua_gc_try_collect(struct grid_lua_model* mod){

    if (lua_gc(mod->L, LUA_GCCOUNT)>70){ //60kb

        lua_gc(mod->L, LUA_GCCOLLECT);
        grid_lua_debug_memory_stats(mod, "gc 70kb");
        mod->dostring_count = 0;

    }


}


void grid_lua_gc_collect(struct grid_lua_model* mod){

    lua_gc(mod->L, LUA_GCCOLLECT);


}

void grid_lua_debug_memory_stats(struct grid_lua_model* mod, char* message){

    uint32_t memusage = lua_gc(grid_lua_state.L, LUA_GCCOUNT)*1024 + lua_gc(grid_lua_state.L, LUA_GCCOUNTB);
    grid_platform_printf("LUA mem usage: %d(%s)\r\n", memusage, message);

}

/*static*/ int grid_lua_panic(lua_State *L) {

    while(1){

        grid_platform_printf("LUA PANIC\r\n");
        grid_platform_delay_ms(1000);
    }

    return 1;
}


/* ==================== LUA C API REGISTERED FUNCTIONS  ====================*/

/*static*/ int l_grid_send(lua_State* L) {

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


/*static*/ int l_grid_midirx_enabled(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    int32_t param[1] = {0};
    uint8_t isgetter = 0;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isnumber(L, i)){
            param[i-1] = lua_tointeger(L, i);  
        }

        
    }

    grid_sys_set_midirx_any_state(&grid_sys_state, (uint8_t) param[0]);

    
    return 1;
}

/*static*/ int l_grid_midirx_sync(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    int32_t param[1] = {0};
    uint8_t isgetter = 0;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isnumber(L, i)){
            param[i-1] = lua_tointeger(L, i);  
        }

        
    }
    
    grid_sys_set_midirx_sync_state(&grid_sys_state, (uint8_t) param[0]);
    
    return 1;
}



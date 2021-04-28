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

static int l_grid_led_set_phase(lua_State* L) {

    int nargs = lua_gettop(L);

   

    if (nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[3] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_phase(&grid_led_state, param[0], param[1], param[2]);

    return 0;
}


static int l_grid_led_set_min(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=5){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[5] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_min(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}

static int l_grid_led_set_mid(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=5){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[5] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_mid(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}

static int l_grid_led_set_max(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=5){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[5] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_max(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}

static int l_grid_led_set_frequency(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[3] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_frequency(&grid_led_state, param[0], param[1], param[2]);
    return 0;
}

static int l_grid_led_set_shape(lua_State* L) {

    

    int nargs = lua_gettop(L);


    if (nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[3] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }


    printf("Led shape %d %d %d\r\n", param[0], param[1], param[2]);

    grid_led_set_shape(&grid_led_state, param[0], param[1], param[2]);


    return 0;
}

static int l_grid_led_set_pfs(lua_State* L) {

    

    int nargs = lua_gettop(L);


    if (nargs!=5){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[5] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }


    //printf("Led shape %d %d %d\r\n", param[0], param[1], param[2]);

    grid_led_set_phase(&grid_led_state, param[0], param[1], param[2]);
    grid_led_set_frequency(&grid_led_state, param[0], param[1], param[3]);
    grid_led_set_shape(&grid_led_state, param[0], param[1], param[4]);


    return 0;
}

static int l_grid_load_template_variables(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[1] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    uint8_t str_to_do[100] = {0};
    sprintf(str_to_do, "this = element[%d]", param[0]);    
    grid_lua_dostring(&grid_lua_state, str_to_do);


    for (uint8_t i=0; i<14; i++){

        int32_t vari = grid_ui_state.bank_list[grid_sys_get_bank_num(&grid_sys_state)].element_list[param[0]].template_parameter_list[i];

        uint8_t str_to_do[100] = {0};
        sprintf(str_to_do, "this.T[%d]=%d", i, vari);
				
        grid_lua_dostring(&grid_lua_state, str_to_do); // +6 is length of "<?lua "
  
        //printf("GRID: %s: %d\r\n", str_to_do, vari);
    }

    return 0;
}

static int l_grid_store_template_variables(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[1] = {0};


    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    lua_pop(L, 2);

    for (uint8_t i=0; i<14; i++){

        uint8_t str_to_do[100] = {0};
        sprintf(str_to_do, "return this.T[%d]", i);		
        grid_lua_dostring(&grid_lua_state, str_to_do); // +6 is length of "<?lua "

        lua_Integer lnum = lua_tointeger(L, -1);
        lua_pop(L, 1);

        grid_ui_state.bank_list[grid_sys_get_bank_num(&grid_sys_state)].element_list[param[0]].template_parameter_list[i] = lnum;

        //printf("LUA: %s: %d\r\n", str_to_do, lnum);

    }

    lua_gc(grid_lua_state.L, LUA_GCCOLLECT);
    return 0;
}

static const struct luaL_Reg printlib [] = {
  {"print", l_my_print},
  {"grid_send", l_grid_send},
  {"grid_led_set_phase", l_grid_led_set_phase},
  {"glsp", l_grid_led_set_phase},
  {"grid_led_set_min", l_grid_led_set_min},
  {"grid_led_set_mid", l_grid_led_set_mid},
  {"grid_led_set_max", l_grid_led_set_max},
  {"grid_led_set_frequency", l_grid_led_set_frequency},
  {"grid_led_set_shape", l_grid_led_set_shape},
  {"grid_led_set_pfs", l_grid_led_set_pfs},
  {"grid_load_template_variables", l_grid_load_template_variables},
  {"grid_store_template_variables", l_grid_store_template_variables},
  
  {NULL, NULL} /* end of array */
};


uint8_t grid_lua_init(struct grid_lua_model* mod){


    mod->stdo_len = GRID_LUA_STDO_LENGTH;
    mod->stdi_len = GRID_LUA_STDI_LENGTH;
    mod->stde_len = GRID_LUA_STDE_LENGTH;

    grid_lua_clear_stdo(mod);
    grid_lua_clear_stdi(mod);
    grid_lua_clear_stde(mod);

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
    grid_lua_dostring(mod, "gsm = grid_send_midi");

    grid_lua_debug_memory_stats(mod, "grid_send");

    grid_lua_ui_init(mod, &grid_sys_state);


}

uint8_t grid_lua_ui_init(struct grid_lua_model* mod, struct grid_sys_model* sys){


    printf("LUA UI INIT HWCFG:%d\r\n", grid_sys_get_hwcfg(sys));


    switch (grid_sys_get_hwcfg(sys)){

        case GRID_MODULE_PO16_RevB: grid_lua_ui_init_po16(mod); break;
        case GRID_MODULE_PO16_RevC: grid_lua_ui_init_po16(mod); break;

        case GRID_MODULE_BU16_RevB: grid_lua_ui_init_bu16(mod); break;
        case GRID_MODULE_BU16_RevC: grid_lua_ui_init_bu16(mod); break;

        case GRID_MODULE_PBF4_RevA: grid_lua_ui_init_pbf4(mod); break;

        case GRID_MODULE_EN16_RevA: grid_lua_ui_init_en16(mod); break;
        case GRID_MODULE_EN16_RevD: grid_lua_ui_init_en16(mod); break;

        case GRID_MODULE_EN16_ND_RevA: grid_lua_ui_init_en16(mod); break;
        case GRID_MODULE_EN16_ND_RevD: grid_lua_ui_init_en16(mod); break;

    }

}

uint8_t grid_lua_ui_init_po16(struct grid_lua_model* mod){

    printf("LUA UI INIT PO16\r\n");
    // define encoder_init_function
    grid_lua_dostring(mod, "init_encoder = function (e) e.T = {} for i=0, 10 do e.T[i] = 0 end end");

    // create element array
    grid_lua_dostring(mod, "element = {} this = {}");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do element[i] = {} init_encoder(element[i]) end");
    
    printf("LUA UI INIT\r\n");
}

uint8_t grid_lua_ui_init_bu16(struct grid_lua_model* mod){

    printf("LUA UI INIT BU16\r\n");
    // define encoder_init_function
    grid_lua_dostring(mod, "init_encoder = function (e) e.T = {} for i=0, 10 do e.T[i] = 0 end end");

    // create element array
    grid_lua_dostring(mod, "element = {} this = {}");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do element[i] = {} init_encoder(element[i]) end");
    
    printf("LUA UI INIT\r\n");
}

uint8_t grid_lua_ui_init_pbf4(struct grid_lua_model* mod){

    printf("LUA UI INIT PBF4\r\n");
    // define encoder_init_function
    grid_lua_dostring(mod, "init_encoder = function (e) e.T = {} for i=0, 10 do e.T[i] = 0 end end");

    // create element array
    grid_lua_dostring(mod, "element = {} this = {}");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do element[i] = {} init_encoder(element[i]) end");
    
    printf("LUA UI INIT\r\n");
}

uint8_t grid_lua_ui_init_en16(struct grid_lua_model* mod){

    printf("LUA UI INIT EN16\r\n");
    // define encoder_init_function
    grid_lua_dostring(mod, "init_encoder = function (e) e.T = {} for i=0, 14 do e.T[i] = 0 end end");

    // create element array
    grid_lua_dostring(mod, "element = {} this = {}");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do element[i] = {} init_encoder(element[i]) end");
    
    printf("LUA UI INIT\r\n");

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



void grid_lua_clear_stde(struct grid_lua_model* mod){


    for (uint32_t i=0; i<mod->stde_len; i++){
        mod->stde[i] = 0;
    }


}


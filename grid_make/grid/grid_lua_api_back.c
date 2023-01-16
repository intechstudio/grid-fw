/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_lua_api_back.h"


int _gettimeofday(){return 0;} 
int _open(){while(1);} 
int _times(){while(1);} 
int _unlink(){while(1);} 
int _link(){while(1);} 



/*static*/ int l_grid_led_layer_phase(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2 && nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    int32_t param[3] = {0};
    uint8_t isgetter = 0;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isnumber(L, i)){
            param[i-1] = lua_tointeger(L, i);  
        }

        
    }

    if (nargs == 3){
        //setter

        if (param[2] > 255)  param[2] = 255;
        if (param[2] > -1){ // phase is a phase value
            grid_led_set_layer_phase(&grid_led_state, param[0], param[1], param[2]);
        }
        else{   // phase == -1 means it should be automatically calculated based on min-max values

            struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);
            enum grid_ui_element_t ele_type = ele->type;

            int32_t min = 0;
            int32_t max = 0;
            int32_t val = 0;

            //grid_platform_printf("Param0: %d ", param[0]);

            if (ele_type == GRID_UI_ELEMENT_POTENTIOMETER){


                
                min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_MIN_index);
                max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_MAX_index);
                val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_VALUE_index);
            }
            else if (ele_type == GRID_UI_ELEMENT_ENCODER){
                
                min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MIN_index);
                max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MAX_index);
                val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index);
            }
            else{

                strcat(grid_lua_state.stde, "#elementNotSupported");
                return 0;
            }


            uint16_t phase = grid_utility_map(val, min, max, 0, 255);
            //grid_platform_printf("LED: %d\r\n", phase);
            grid_led_set_layer_phase(&grid_led_state, param[0], param[1], phase);

        }


    }
    else{
        //getter
        int32_t var = grid_led_get_layer_phase(&grid_led_state, param[0], param[1]);
        lua_pushinteger(L, var);
    }
    
    return 1;
}

/*static*/ int l_grid_led_layer_pfs(lua_State* L) {

    

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


    //grid_platform_printf("Led shape %d %d %d\r\n", param[0], param[1], param[2]);

    grid_led_set_layer_phase(&grid_led_state, param[0], param[1], param[2]);
    grid_led_set_layer_frequency(&grid_led_state, param[0], param[1], param[3]);
    grid_led_set_layer_shape(&grid_led_state, param[0], param[1], param[4]);


    return 0;
}

/*static*/ int l_grid_template_variable(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2 && nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    int32_t param[3] = {0};

    uint8_t isgetter = 0;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isinteger(L, i)){
            
        }
        else if (lua_isnil(L, i)){
            // grid_platform_printf(" %d : NIL ", i);
            if (i==3){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }
    //lua_pop(L, 2);

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

    if (ele != NULL){    
        
        uint8_t template_index = param[1];


        if (isgetter){

            int32_t var = grid_ui_element_get_template_parameter(ele, template_index);
            lua_pushinteger(L, var);

        }
        else{

            int32_t var =  param[2];
            grid_ui_element_set_template_parameter(ele, template_index, var);

        }
    

    }

    return 1;
}

/*static*/ int l_grid_page_next(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
    
    
    uint8_t page = grid_ui_page_get_next(&grid_ui_state);
    lua_pushinteger(L, page);
    
    return 1;
}

/*static*/ int l_grid_page_prev(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
           
           
    uint8_t page = grid_ui_page_get_prev(&grid_ui_state);
    lua_pushinteger(L, page);
    
    return 1;
}

/*static*/ int l_grid_page_curr(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
           
    uint8_t page = grid_ui_get_activepage(&grid_ui_state);
    lua_pushinteger(L, page);
    
    return 1;
}
/*static*/ int l_grid_page_load(lua_State* L) {

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

    uint8_t page = param[0];
   
    if (grid_ui_page_change_is_enabled(&grid_ui_state)){

        if (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state) == 0){

            grid_port_debug_printf("page request: %d", page);
            uint8_t response[20] = {0};
            sprintf(response, GRID_CLASS_PAGEACTIVE_frame);
            grid_msg_string_set_parameter(response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);
            grid_msg_string_set_parameter(response, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, page, NULL);
            strcat(grid_lua_state.stdo, response);

        }
        else{
            //grid_platform_printf("page change in progress \r\n");
        }

    }
    else{
        //grid_platform_printf("page change is disabled\r\n");
        grid_port_debug_printf("page change is disabled");
    	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);
    }


    return 1;
}

/*static*/ int l_grid_timer_start(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint32_t param[2] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }


    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

    if (ele != NULL){

        grid_ui_element_timer_set(ele, param[1]*RTC1MS);

    }  
    else{

        strcat(grid_lua_state.stde, "#invalidRange");
    }


    return 1;
}


/*static*/ int l_grid_timer_stop(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint32_t param[1] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }



    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

    if (ele != NULL){

        grid_ui_element_timer_set(ele, 0);

    }  
    else{

        strcat(grid_lua_state.stde, "#invalidRange");
    }

    return 1;
}

/*static*/ int l_grid_event_trigger(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint32_t param[2] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }


    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

    if (ele != NULL){

        struct grid_ui_event* eve = grid_ui_event_find(ele, param[1]);

        if (eve != NULL){

            grid_ui_event_trigger(eve);
        }
        else{
            strcat(grid_lua_state.stde, "#invalidEvent");
        }

    }  
    else{

        strcat(grid_lua_state.stde, "#invalidRange");
    }


    return 1;
}


/*static*/ const struct luaL_Reg printlib [] = {
    {"print", l_my_print},
    {"grid_send", l_grid_send},

    {GRID_LUA_FNC_G_LED_RED_short,        GRID_LUA_FNC_G_LED_RED_fnptr},
    {GRID_LUA_FNC_G_LED_GRE_short,        GRID_LUA_FNC_G_LED_GRE_fnptr},
    {GRID_LUA_FNC_G_LED_BLU_short,        GRID_LUA_FNC_G_LED_BLU_fnptr},

    {GRID_LUA_FNC_G_LED_PHASE_short,        GRID_LUA_FNC_G_LED_PHASE_fnptr},
    {GRID_LUA_FNC_G_LED_MIN_short,          GRID_LUA_FNC_G_LED_MIN_fnptr},
    {GRID_LUA_FNC_G_LED_MID_short,          GRID_LUA_FNC_G_LED_MID_fnptr},
    {GRID_LUA_FNC_G_LED_MAX_short,          GRID_LUA_FNC_G_LED_MAX_fnptr},
    {GRID_LUA_FNC_G_LED_COLOR_short,        GRID_LUA_FNC_G_LED_COLOR_fnptr},
    {GRID_LUA_FNC_G_LED_FREQUENCY_short,    GRID_LUA_FNC_G_LED_FREQUENCY_fnptr},
    {GRID_LUA_FNC_G_LED_SHAPE_short,        GRID_LUA_FNC_G_LED_SHAPE_fnptr},
    {GRID_LUA_FNC_G_LED_TIMEOUT_short,      GRID_LUA_FNC_G_LED_TIMEOUT_fnptr},
    {GRID_LUA_FNC_G_LED_PSF_short,          GRID_LUA_FNC_G_LED_PSF_fnptr},

    {GRID_LUA_FNC_G_MIDI_SEND_short,        GRID_LUA_FNC_G_MIDI_SEND_fnptr},
    {GRID_LUA_FNC_G_MIDISYSEX_SEND_short,   GRID_LUA_FNC_G_MIDISYSEX_SEND_fnptr},

    {GRID_LUA_FNC_G_MIDIRX_ENABLED_short,   GRID_LUA_FNC_G_MIDIRX_ENABLED_fnptr},
    {GRID_LUA_FNC_G_MIDIRX_SYNC_short,      GRID_LUA_FNC_G_MIDIRX_SYNC_fnptr},

    {GRID_LUA_FNC_G_KEYBOARD_SEND_short,    GRID_LUA_FNC_G_KEYBOARD_SEND_fnptr},

    {GRID_LUA_FNC_G_MOUSEMOVE_SEND_short,   GRID_LUA_FNC_G_MOUSEMOVE_SEND_fnptr},
    {GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short, GRID_LUA_FNC_G_MOUSEBUTTON_SEND_fnptr},

    {GRID_LUA_FNC_G_VERSION_MAJOR_short,    GRID_LUA_FNC_G_VERSION_MAJOR_fnptr},
    {GRID_LUA_FNC_G_VERSION_MINOR_short,    GRID_LUA_FNC_G_VERSION_MINOR_fnptr},
    {GRID_LUA_FNC_G_VERSION_PATCH_short,    GRID_LUA_FNC_G_VERSION_PATCH_fnptr},

    {GRID_LUA_FNC_G_MODULE_POSX_short,    GRID_LUA_FNC_G_MODULE_POSX_fnptr},
    {GRID_LUA_FNC_G_MODULE_POSY_short,    GRID_LUA_FNC_G_MODULE_POSY_fnptr},
    {GRID_LUA_FNC_G_MODULE_ROT_short,    GRID_LUA_FNC_G_MODULE_ROT_fnptr},

    {GRID_LUA_FNC_G_PAGE_NEXT_short,    GRID_LUA_FNC_G_PAGE_NEXT_fnptr},
    {GRID_LUA_FNC_G_PAGE_PREV_short,    GRID_LUA_FNC_G_PAGE_PREV_fnptr},
    {GRID_LUA_FNC_G_PAGE_CURR_short,    GRID_LUA_FNC_G_PAGE_CURR_fnptr},
    {GRID_LUA_FNC_G_PAGE_LOAD_short,    GRID_LUA_FNC_G_PAGE_LOAD_fnptr},

    {GRID_LUA_FNC_G_TIMER_START_short,    GRID_LUA_FNC_G_TIMER_START_fnptr},
    {GRID_LUA_FNC_G_TIMER_STOP_short,    GRID_LUA_FNC_G_TIMER_STOP_fnptr},

    {GRID_LUA_FNC_G_EVENT_TRIGGER_short,    GRID_LUA_FNC_G_EVENT_TRIGGER_fnptr},

    {GRID_LUA_FNC_G_HWCFG_short,    GRID_LUA_FNC_G_HWCFG_fnptr},

    {GRID_LUA_FNC_G_RANDOM_short,    GRID_LUA_FNC_G_RANDOM_fnptr},
    {GRID_LUA_FNC_G_ELEMENTNAME_SEND_short, GRID_LUA_FNC_G_ELEMENTNAME_SEND_fnptr},

    {GRID_LUA_FNC_G_WEBSOCKET_SEND_short, GRID_LUA_FNC_G_WEBSOCKET_SEND_fnptr},

    {"print", l_my_print},
  
    {"gtv", l_grid_template_variable},
  
    {NULL, NULL} /* end of array */
};

void grid_lua_start_vm(struct grid_lua_model* mod){

	mod->L = luaL_newstate();

    lua_atpanic(mod->L, &grid_lua_panic);

    grid_lua_debug_memory_stats(mod, "Init");


    luaL_openlibs(mod->L);

    grid_lua_debug_memory_stats(mod, "Openlibs");

    grid_lua_dostring(mod, GRID_LUA_GLUT_source);
    grid_lua_dostring(mod, GRID_LUA_GLIM_source);    
    grid_lua_dostring(mod, GRID_LUA_GEN_source);
    grid_lua_dostring(mod, "midi = {}");
    grid_lua_dostring(mod, "midi.ch = 0 midi.cmd=176 midi.p1=0 midi.p2=0");
    grid_lua_dostring(mod, "midi.send_packet = function (self,ch,cmd,p1,p2) "GRID_LUA_FNC_G_MIDI_SEND_short"(ch,cmd,p1,p2) end");

    grid_lua_dostring(mod, "mouse = {}");
    grid_lua_dostring(mod, "mouse.send_axis_move = function (self,p,a) "GRID_LUA_FNC_G_MOUSEMOVE_SEND_short"(p,a) end");
    grid_lua_dostring(mod, "mouse.send_button_change = function (self,s,b) "GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short"(s,b) end");
 
    grid_lua_dostring(mod, "keyboard = {}");
    grid_lua_dostring(mod, "keyboard.send_macro = function (self,...) "GRID_LUA_FNC_G_KEYBOARD_SEND_short"(...) end");




    lua_getglobal(mod->L, "_G");
	luaL_setfuncs(mod->L, printlib, 0);


	lua_pop(mod->L, 1);
    grid_lua_debug_memory_stats(mod, "Printlib");




    grid_lua_ui_init(mod, &grid_sys_state);
    grid_lua_debug_memory_stats(mod, "Ui init");



}


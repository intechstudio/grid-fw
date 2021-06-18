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



static int l_grid_keyboard_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if ((nargs-1)%3 != 0 || nargs == 0){

        printf("kb invalid params %d\r\n", nargs);
        return 0;
    }


    uint8_t temp[100] = {0};
    sprintf(temp, GRID_CLASS_HIDKEYBOARD_frame_start);

    grid_msg_set_parameter(temp, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    uint8_t cursor = 0;

    uint8_t default_delay = lua_tonumber(L, 1);

    grid_msg_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length, default_delay, NULL);

    for (int i=2; i <= nargs; i+=3) {

        int32_t modifier = lua_tonumber(L, i);
        int32_t keystate = lua_tonumber(L, i+1);

        if (modifier > 15 || modifier<0){printf("invalid modifier param %d\r\n", modifier); continue;}
        if (!(keystate==0 || keystate==1 || keystate==2)){printf("invalid keystate param %d\r\n", keystate); continue;}

        if (modifier == 15){
            // delay command
            int32_t delay = lua_tonumber(L, i+2);

            if (delay > 4095) delay = 4095;
            if (delay < 0) delay = 0;
    
            grid_msg_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
            grid_msg_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length, delay, NULL);   

        }
        else if (modifier == 0 || modifier == 1){
            // normal key or modifier
            int32_t keycode = lua_tonumber(L, i+2);

            grid_msg_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
            grid_msg_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset, GRID_CLASS_HIDKEYBOARD_KEYSTATE_length, keystate, NULL);
            grid_msg_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYCODE_offset, GRID_CLASS_HIDKEYBOARD_KEYCODE_length, keycode, NULL);   

        }
        else{

            continue;
        }
    
        cursor += 4;

    }

    grid_msg_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_LENGTH_offset, GRID_CLASS_HIDKEYBOARD_LENGTH_length, cursor/3*4, NULL);
         
    temp[strlen(temp)] = GRID_CONST_ETX;

    if (cursor != 1){
        strcat(grid_lua_state.stdo, temp);
        //printf("keyboard: %s\r\n", temp); 
    }
    else{
        printf("invalid args!\r\n");
        return 0;
    }


    return 1;
}


static int l_my_print(lua_State* L) {

    int nargs = lua_gettop(L);
    //printf("LUA PRINT: ");
    for (int i=1; i <= nargs; ++i) {

        if (lua_type(L, i) == LUA_TSTRING){
            
            grid_debug_printf("%s", lua_tostring(L, i));
		    //printf(" str: %s ", lua_tostring(L, i));
        }
        else if (lua_type(L, i) == LUA_TNUMBER){

            lua_Number lnum = lua_tonumber(L, i);
            lua_Integer lint;
            lua_numbertointeger(lnum, &lint);
            //int32_t num = lua_tonumber

            grid_debug_printf("%d", (int)lnum);
		    //printf(" num: %d ", (int)lnum);
        }
        else if (lua_type(L, i) == LUA_TNIL){
            //printf(" nil ");
        }
        else if (lua_type(L, i) == LUA_TFUNCTION){
            //printf(" fnc ");
        }
        else if (lua_type(L, i) == LUA_TTABLE){
            //printf(" table ");
        }
        else{
            //printf(" unknown data type ");
        }
    }

    if (nargs == 0){
        //printf(" no arguments ");
    }

    //printf("\r\n");

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



static int l_grid_midi_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=4){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[4] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    uint8_t channel = param[0];
    uint8_t command = param[1];
    uint8_t param1 = param[2];
    uint8_t param2 = param[3];


    uint8_t midiframe[20] = {0};

    sprintf(midiframe, GRID_CLASS_MIDI_frame);

    grid_msg_set_parameter(midiframe, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    grid_msg_set_parameter(midiframe, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, channel, NULL);
    grid_msg_set_parameter(midiframe, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, command, NULL);
    grid_msg_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, param1, NULL);
    grid_msg_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, param2, NULL);
    
    //printf("MIDI: %s\r\n", midiframe);  
    strcat(grid_lua_state.stdo, midiframe);

    return 1;
}


static int l_grid_led_phase(lua_State* L) {

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
        if (param[2] < 0)    param[2] = 0;

        grid_led_set_phase(&grid_led_state, param[0], param[1], param[2]);

    }
    else{
        //getter
        int32_t var = grid_led_get_phase(&grid_led_state, param[0], param[1]);
        lua_pushinteger(L, var);
    }
    
    return 1;
}


static int l_grid_led_set_phase(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs == 2){  // automatically set phase to element value


        uint8_t param[2] = {0};

        for (int i=1; i <= nargs; ++i) {
            param[i-1] = lua_tointeger(L, i);
        }

        if (param[0]<grid_ui_state.element_list_length){
	        
            struct grid_ui_element* ele = &grid_ui_state.element_list[param[0]];
            enum grid_ui_element_t ele_type = ele->type;

            int32_t min = 0;
            int32_t max = 0;
            int32_t val = 0;

            //printf("Param0: %d ", param[0]);

            if (ele_type == GRID_UI_ELEMENT_POTENTIOMETER){

                min = ele->template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
                max = ele->template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];
                val = ele->template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index];
            }
            else if (ele_type == GRID_UI_ELEMENT_ENCODER){
                
                min = ele->template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
                max = ele->template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];
                val = ele->template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];
            }
            else{

                strcat(grid_lua_state.stde, "#elementNotSupported");
                return 0;
            }

            uint16_t phase = grid_utility_map(val, min, max, 0, 255);
            //printf("LED: %d\r\n", phase);
            grid_led_set_phase(&grid_led_state, param[0], param[1], phase);


        }


        return 0;


    }
    else if (nargs == 3){  // manually set phase to arbitery value

        uint8_t param[3] = {0};

        for (int i=1; i <= nargs; ++i) {
            param[i-1] = lua_tointeger(L, i);
        }

        grid_led_set_phase(&grid_led_state, param[0], param[1], param[2]);

        return 0;

    }
    else
    {
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

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

static int l_grid_led_set_color(lua_State* L) {

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

    grid_led_set_color(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


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


    //printf("Led shape %d %d %d\r\n", param[0], param[1], param[2]);

    grid_led_set_shape(&grid_led_state, param[0], param[1], param[2]);


    return 0;
}

static int l_grid_led_set_timeout(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=3){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint16_t param[3] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    grid_led_set_timeout(&grid_led_state, (uint8_t)param[0], (uint8_t)param[1], param[2]);
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


static int l_grid_template_variable(lua_State* L) {

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
            // printf(" %d : NIL ", i);
            if (i==3){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }
    //lua_pop(L, 2);

    if (isgetter){

        int32_t var = grid_ui_state.element_list[param[0]].template_parameter_list[param[1]];
        // printf("GTV Getter: %d %d %d : %d\r\n", param[0], param[1], param[2], var);
        lua_pushinteger(L, var);

    }
    else{
        int32_t var =  param[2];
        grid_ui_state.element_list[param[0]].template_parameter_list[param[1]] = var;
        // printf("GTV Setter: %d %d %d : %d\r\n", param[0], param[1], param[2], var);
        //lua_pushinteger(L, var);
    }
    
    return 1;
}


static int l_led_default_red(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0 && nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#LED.invalidParams");
        return 0;
    }

    uint8_t param[1] = {0};

    uint8_t isgetter = 1;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isinteger(L, i)){
            isgetter = 0;
        }
        else if (lua_isnil(L, i)){
            // printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_state.bank_activebank_color_r;
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_state.bank_activebank_color_r = var;
    }
    
    return 1;
}
static int l_led_default_green(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0 && nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#LED.invalidParams");
        return 0;
    }

    uint8_t param[1] = {0};

    uint8_t isgetter = 1;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isinteger(L, i)){
            isgetter = 0;
        }
        else if (lua_isnil(L, i)){
            // printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_state.bank_activebank_color_g;
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_state.bank_activebank_color_g = var;
    }
    
    return 1;
}
static int l_led_default_blue(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0 && nargs!=1){
        // error
        strcat(grid_lua_state.stde, "#LED.invalidParams");
        return 0;
    }

    uint8_t param[1] = {0};

    uint8_t isgetter = 1;

    for (int i=1; i <= nargs; ++i) {

        if (lua_isinteger(L, i)){
            isgetter = 0;
        }
        else if (lua_isnil(L, i)){
            // printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_state.bank_activebank_color_b;
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_state.bank_activebank_color_b = var;
    }
    
    return 1;
}

static int l_grid_version_major(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_MAJOR);
    
    return 1;
}


static int l_grid_version_minor(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_MINOR);
    
    return 1;
}

static int l_grid_version_patch(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_PATCH);
    
    return 1;
}

static int l_grid_hwcfg(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_get_hwcfg(&grid_sys_state));
    
    return 1;
}

static int l_grid_random(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, rand_sync_read8(&RAND_0));
    
    return 1;
}

static int l_grid_position_x(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_state.module_x);
    
    return 1;
}

static int l_grid_position_y(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_state.module_y);
    
    return 1;
}

static int l_grid_rotation(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_state.module_rot);
    
    return 1;
}

static int l_grid_page_next(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
    
    uint8_t page = (grid_ui_state.page_activepage + 1) % grid_ui_state.page_count;
    lua_pushinteger(L, page);
    
    return 1;
}

static int l_grid_page_prev(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
           
    uint8_t page = (grid_ui_state.page_activepage + grid_ui_state.page_count - 1) % grid_ui_state.page_count;
    lua_pushinteger(L, page);
    
    return 1;
}

static int l_grid_page_curr(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }
           
    uint8_t page = (grid_ui_state.page_activepage);
    lua_pushinteger(L, page);
    
    return 1;
}
static int l_grid_page_load(lua_State* L) {

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
   
    if (grid_ui_state.page_change_enabled == 1){

        if (grid_nvm_ui_bulk_read_is_in_progress(&grid_nvm_state, &grid_ui_state) == 0){

            grid_debug_printf("page request: %d", page);
            uint8_t response[20] = {0};
            sprintf(response, GRID_CLASS_PAGEACTIVE_frame);
            grid_msg_set_parameter(response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);
            grid_msg_set_parameter(response, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, page, NULL);
            strcat(grid_lua_state.stdo, response);

        }
        else{
            printf("rip \r\n");
        }

    }
    else{
        //printf("page change is disabled\r\n");
        grid_debug_printf("page change is disabled");
    	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);
    }


    return 1;
}


static const struct luaL_Reg printlib [] = {
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

    {GRID_LUA_FNC_G_KEYBOARD_SEND_short,        GRID_LUA_FNC_G_KEYBOARD_SEND_fnptr},

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

    {GRID_LUA_FNC_G_HWCFG_short,    GRID_LUA_FNC_G_HWCFG_fnptr},

    {GRID_LUA_FNC_G_RANDOM_short,    GRID_LUA_FNC_G_RANDOM_fnptr},
  
    {"gtv", l_grid_template_variable},
  
    {NULL, NULL} /* end of array */
};



uint8_t grid_lua_init(struct grid_lua_model* mod){


    mod->stdo_len = GRID_LUA_STDO_LENGTH;
    mod->stdi_len = GRID_LUA_STDI_LENGTH;
    mod->stde_len = GRID_LUA_STDE_LENGTH;

    grid_lua_clear_stdo(mod);
    grid_lua_clear_stdi(mod);
    grid_lua_clear_stde(mod);

    mod->dostring_count = 0;

}

uint8_t grid_lua_deinit(struct grid_lua_model* mod){


}

uint8_t grid_lua_debug_memory_stats(struct grid_lua_model* mod, char* message){

    uint32_t memusage = lua_gc(grid_lua_state.L, LUA_GCCOUNT)*1024 + lua_gc(grid_lua_state.L, LUA_GCCOUNTB);
    printf("LUA mem usage: %d(%s)\r\n", memusage, message);

}


uint8_t grid_lua_start_vm(struct grid_lua_model* mod){

	mod->L = luaL_newstate();

    lua_atpanic(mod->L, &grid_lua_panic);

    grid_lua_debug_memory_stats(mod, "Init");


    luaL_openlibs(mod->L);

    grid_lua_debug_memory_stats(mod, "Openlibs");

    grid_lua_dostring(mod, GRID_LUA_GLUT_source);
    grid_lua_dostring(mod, GRID_LUA_GLIM_source);

    lua_getglobal(mod->L, "_G");
	luaL_setfuncs(mod->L, printlib, 0);
	lua_pop(mod->L, 1);
    grid_lua_debug_memory_stats(mod, "Printlib");




    grid_lua_ui_init(mod, &grid_sys_state);
    grid_lua_debug_memory_stats(mod, "Ui init");


}

uint8_t grid_lua_ui_init(struct grid_lua_model* mod, struct grid_sys_model* sys){


    printf("LUA UI INIT HWCFG:%d\r\n", grid_sys_get_hwcfg(sys));

    //register init functions for different ui elements

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

    grid_lua_dostring(mod, GRID_LUA_P_LIST_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} "GRID_LUA_KW_THIS_short" = {}");

    // initialize 16 potmeters
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {} init_potmeter(ele[i], i) end");

    grid_lua_dostring(mod, GRID_LUA_P_LIST_deinit);
    printf("LUA UI INIT\r\n");
}

uint8_t grid_lua_ui_init_bu16(struct grid_lua_model* mod){

     printf("LUA UI INIT BU16\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_B_LIST_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} "GRID_LUA_KW_THIS_short" = {}");

    // initialize 16 buttons
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {} init_button(ele[i], i) end");

    grid_lua_dostring(mod, GRID_LUA_B_LIST_deinit);
    printf("LUA UI INIT\r\n");
}

uint8_t grid_lua_ui_init_pbf4(struct grid_lua_model* mod){

     printf("LUA UI INIT PBF4\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_P_LIST_init);
    grid_lua_dostring(mod, GRID_LUA_B_LIST_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} "GRID_LUA_KW_THIS_short" = {}");

    // initialize 8 potmeters and 4 buttons
    grid_lua_dostring(mod, "for i=0, 7  do "GRID_LUA_KW_ELEMENT_short"[i] = {} init_potmeter(ele[i], i) end");
    grid_lua_dostring(mod, "for i=8, 11 do "GRID_LUA_KW_ELEMENT_short"[i] = {} init_button(ele[i], i) end");

    grid_lua_dostring(mod, GRID_LUA_P_LIST_deinit);
    grid_lua_dostring(mod, GRID_LUA_B_LIST_deinit);
    printf("LUA UI INIT\r\n");

}




uint8_t grid_lua_ui_init_en16(struct grid_lua_model* mod){

    printf("LUA UI INIT EN16\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_E_LIST_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} "GRID_LUA_KW_THIS_short" = {}");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {} init_encoder(ele[i], i) end");

    grid_lua_dostring(mod, GRID_LUA_E_LIST_deinit);
    printf("LUA UI INIT\r\n");

}


uint8_t grid_lua_stop_vm(struct grid_lua_model* mod){

    lua_close(mod->L);
}



void grid_lua_gc_try_collect(struct grid_lua_model* mod){

    if (lua_gc(mod->L, LUA_GCCOUNT)>60){ //60kb

        lua_gc(mod->L, LUA_GCCOLLECT);
        grid_lua_debug_memory_stats(mod, "gc 60kb");
        mod->dostring_count = 0;

    }


}

uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code){

    mod->dostring_count++;

    if (luaL_loadstring(mod->L, code) == LUA_OK){

        if (( lua_pcall(mod->L, 0, LUA_MULTRET, 0)) == LUA_OK) {
            // If it was executed successfuly we 
            // remove the code from the stack

        }
        else{
            printf("LUA not OK: %s \r\n", code);
            grid_debug_printf("LUA not OK");
        }

        lua_pop(mod->L, lua_gettop(mod->L));
    
    }
    else{
        printf("LUA not OK:  %s\r\n", code);
        grid_debug_printf("LUA not OK");
    }


    grid_lua_gc_try_collect(mod);

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


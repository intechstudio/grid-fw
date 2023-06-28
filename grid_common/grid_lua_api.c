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

    mod->L = NULL;

    mod->target_memory_usage_kilobytes = 70; // 70kb

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

void grid_lua_set_memory_target(struct grid_lua_model* mod, uint8_t target_kilobytes){
    mod->target_memory_usage_kilobytes = target_kilobytes;
}

uint8_t grid_lua_get_memory_target(struct grid_lua_model* mod){
    return mod->target_memory_usage_kilobytes;
}


void grid_lua_gc_try_collect(struct grid_lua_model* mod){

    uint8_t target_kilobytes = grid_lua_get_memory_target(mod);

    if (lua_gc(mod->L, LUA_GCCOUNT) > target_kilobytes){

        lua_gc(mod->L, LUA_GCCOLLECT);

        char message[10] = {0};
        sprintf(message, "gc %dkb", target_kilobytes);
        grid_lua_debug_memory_stats(mod, message);
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


/*static*/ int32_t grid_utility_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* ==================== LUA C API REGISTERED FUNCTIONS  ====================*/


/*static*/ int l_my_print(lua_State* L) {

    int nargs = lua_gettop(L);
    //grid_platform_printf("LUA PRINT: ");
    for (int i=1; i <= nargs; ++i) {

        if (lua_type(L, i) == LUA_TSTRING){
            
            grid_port_debug_printf("%s", lua_tostring(L, i));
		    //grid_platform_printf(" str: %s ", lua_tostring(L, i));
        }
        else if (lua_type(L, i) == LUA_TNUMBER){

            lua_Number lnum = lua_tonumber(L, i);
            lua_Integer lint;
            lua_numbertointeger(lnum, &lint);
            //int32_t num = lua_tonumber

            grid_port_debug_printf("%d", (int)lnum);
		    //grid_platform_printf(" num: %d ", (int)lnum);
        }
        else if (lua_type(L, i) == LUA_TNIL){
            //grid_platform_printf(" nil ");
        }
        else if (lua_type(L, i) == LUA_TFUNCTION){
            //grid_platform_printf(" fnc ");
        }
        else if (lua_type(L, i) == LUA_TTABLE){
            //grid_platform_printf(" table ");
        }
        else{
            //grid_platform_printf(" unknown data type ");
        }
    }

    if (nargs == 0){
        //grid_platform_printf(" no arguments ");
    }

    //grid_platform_printf("\r\n");

    return 0;
}


/*static*/ int l_grid_websocket_send(lua_State* L) {

    char message[500] = {0};


    int nargs = lua_gettop(L);
    //grid_platform_printf("LUA PRINT: ");
    for (int i=1; i <= nargs; ++i) {

        if (lua_type(L, i) == LUA_TSTRING){
            
            strcat(message, lua_tostring(L, i));
		    //grid_platform_printf(" str: %s ", lua_tostring(L, i));
        }
        else if (lua_type(L, i) == LUA_TNUMBER){

            lua_Number lnum = lua_tonumber(L, i);
            lua_Integer lint;
            lua_numbertointeger(lnum, &lint);
            //int32_t num = lua_tonumber

            sprintf(&message[strlen(message)], "%lf", lnum);


            // remove unnesesery trailing zeros
            uint8_t index_helper = strlen(message);
            for (uint8_t i=0; i<8; i++){

                if (message[index_helper-i-1] == '0'){

                    message[index_helper-i-1] = '\0';
                    
                }
                else if (message[index_helper-i-1] == '.'){

                    message[index_helper-i-1] = '\0';
                    break;

                }
                else{
                    break;
                }

            }

		    //grid_platform_printf(" num: %d ", (int)lnum);
        }
        else if (lua_type(L, i) == LUA_TNIL){
            //grid_platform_printf(" nil ");
        }
        else if (lua_type(L, i) == LUA_TFUNCTION){
            //grid_platform_printf(" fnc ");
        }
        else if (lua_type(L, i) == LUA_TTABLE){
            //grid_platform_printf(" table ");
        }
        else{
            //grid_platform_printf(" unknown data type ");
        }
    }

    if (nargs == 0){
        //grid_platform_printf(" no arguments ");
    }

    //grid_platform_printf("\r\n");

    grid_port_websocket_print_text(message);

    return 0;
}

/*static*/ int l_grid_elementname_send(lua_State* L) {

    int nargs = lua_gettop(L);
    //grid_platform_printf("LUA PRINT: ");
    if (nargs == 2){

        if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING){



            uint8_t number = (uint8_t)lua_tointeger(L, 1);
            char string[20] = {0};

            strncpy(string, lua_tostring(L, 2), 19);


            char frame[30] = {0};
            sprintf(frame, GRID_CLASS_ELEMENTNAME_frame_start);

            grid_msg_string_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

            grid_msg_string_set_parameter(frame, GRID_CLASS_ELEMENTNAME_NUM_offset, GRID_CLASS_ELEMENTNAME_NUM_length, number, NULL);
            grid_msg_string_set_parameter(frame, GRID_CLASS_ELEMENTNAME_LENGTH_offset, GRID_CLASS_ELEMENTNAME_LENGTH_length, strlen(string), NULL);
            strcpy(&frame[GRID_CLASS_ELEMENTNAME_NAME_offset], string);
            sprintf(&frame[strlen(frame)], GRID_CLASS_ELEMENTNAME_frame_end);

            strcat(grid_lua_state.stdo, frame);


            // MUST BE SENT OUT IMEDIATELY (NOT THROUGH STDO) BECAUSE IT MUST BE SENT OUT EVEN AFTER LOCAL TRIGGER (CONFIG)
            // struct grid_msg_packet response;
                                    
            // grid_msg_packet_init(&grid_msg_state, &response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

            // uint8_t response_payload[50] = {0};

            // grid_msg_packet_body_append_grid_platform_printf(&response, GRID_CLASS_ELEMENTNAME_frame_start);
                
            // grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);	
            // grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_ELEMENTNAME_NUM_offset, GRID_CLASS_ELEMENTNAME_NUM_length, number);	
            // grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_ELEMENTNAME_LENGTH_offset, GRID_CLASS_ELEMENTNAME_LENGTH_length, strlen(string));					

            // grid_msg_packet_body_append_grid_platform_printf(&response, "%s", string);
            // grid_msg_packet_body_append_grid_platform_printf(&response, GRID_CLASS_ELEMENTNAME_frame_end);    

            // grid_msg_packet_close(&grid_msg_state, &response);
            // grid_port_packet_send_everywhere(&response);


            //grid_port_debug_printf("SN: %d, %s", number, string);
		    //grid_platform_printf(" str: %s ", lua_tostring(L, i));
        }
        else{
            grid_port_debug_printf("Invalid args");
        }

    }
    else{
        grid_port_debug_printf("Invalid args");
    }

    return 0;
}



/*static*/ int l_grid_keyboard_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if ((nargs-1)%3 != 0 || nargs == 0){

        grid_platform_printf("kb invalid params %d\r\n", nargs);
        return 0;
    }


    char temp[20+nargs*4];
    memset(temp, 0x00, 20+nargs*4);
    sprintf(temp, GRID_CLASS_HIDKEYBOARD_frame_start);

    grid_msg_string_set_parameter(temp, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    uint16_t cursor = 0;

    uint8_t default_delay = lua_tonumber(L, 1);

    grid_msg_string_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length, default_delay, NULL);

    uint8_t cnt = 0;

    for (int i=2; i <= nargs; i+=3) {

        cnt++;

        int32_t modifier = lua_tonumber(L, i);
        int32_t keystate = lua_tonumber(L, i+1);

        if (modifier > 15 || modifier<0){grid_platform_printf("invalid modifier param %d\r\n", modifier); continue;}
        if (!(keystate==0 || keystate==1 || keystate==2)){grid_platform_printf("invalid keystate param %d\r\n", keystate); continue;}

        if (modifier == 15){
            // delay command
            int32_t delay = lua_tonumber(L, i+2);

            if (delay > 4095) delay = 4095;
            if (delay < 0) delay = 0;
    
            grid_msg_string_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
            grid_msg_string_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length, delay, NULL);   
            cursor += 4;

        }
        else if (modifier == 0 || modifier == 1){
            // normal key or modifier
            int32_t keycode = lua_tonumber(L, i+2);

            //01234567890123456789012grid_platform_printf("%d-%d ", cnt, keycode);
            cursor += 4;

            grid_msg_string_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
            grid_msg_string_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset, GRID_CLASS_HIDKEYBOARD_KEYSTATE_length, keystate, NULL);
            grid_msg_string_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYCODE_offset, GRID_CLASS_HIDKEYBOARD_KEYCODE_length, keycode, NULL);   

        }
        else{

            continue;
        }
    


    }

    grid_msg_string_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_LENGTH_offset, GRID_CLASS_HIDKEYBOARD_LENGTH_length, cursor/4+1, NULL);
         
    temp[strlen(temp)] = GRID_CONST_ETX;

    if (cursor != 1){
        strcat(grid_lua_state.stdo, temp);
        //grid_platform_printf("keyboard: %s\r\n", temp); 
    }
    else{
        grid_platform_printf("invalid args!\r\n");
        return 0;
    }


    return 1;
}



/*static*/ int l_grid_mousemove_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    int32_t param[2] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    int32_t position_raw = param[0]+128;
    uint8_t axis_raw = param[1];

    
    uint8_t position;
    uint8_t axis;

    if (position_raw>255){
        position = 255;
        strcat(grid_lua_state.stde, "#positionOutOfRange");
    }
    else if (position_raw<0){
        position = 0;
        strcat(grid_lua_state.stde, "#positionOutOfRange");
    }
    else{
        position = position_raw;
    }

    if (axis_raw > 3){
        strcat(grid_lua_state.stde, "#axisOutOfRange");
        axis = 3;
    }
    else if (axis_raw < 1){
        strcat(grid_lua_state.stde, "#axisOutOfRange");
        axis = 1;
    }
    else{
        axis = axis_raw;
    }

    char frame[20] = {0};

    sprintf(frame, GRID_CLASS_HIDMOUSEMOVE_frame);

    grid_msg_string_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    grid_msg_string_set_parameter(frame, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length, position, NULL);
    grid_msg_string_set_parameter(frame, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset, GRID_CLASS_HIDMOUSEMOVE_AXIS_length, axis, NULL);

    strcat(grid_lua_state.stdo, frame);

    return 1;
}

/*static*/ int l_grid_mousebutton_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=2){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    uint8_t param[2] = {0};

    for (int i=1; i <= nargs; ++i) {
        param[i-1] = lua_tointeger(L, i);
    }

    int32_t state_raw = param[0];
    uint8_t button_raw = param[1];

    
    uint8_t state;
    uint8_t button;

    if (state_raw>1){
        state = 1;
        strcat(grid_lua_state.stde, "#stateOutOfRange");
    }
    else if (state_raw<0){
        state = 0;
        strcat(grid_lua_state.stde, "#stateOutOfRange");
    }
    else{
        state = state_raw;
    }

    if (button_raw > 4){
        strcat(grid_lua_state.stde, "#buttonOutOfRange");
        button = 4;
    }
    else if (button_raw < 1){
        strcat(grid_lua_state.stde, "#buttonOutOfRange");
        button = 1;
    }
    else{
        button = button_raw;
    }

    char frame[20] = {0};

    sprintf(frame, GRID_CLASS_HIDMOUSEBUTTON_frame);

    grid_msg_string_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    grid_msg_string_set_parameter(frame, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length, state, NULL);
    grid_msg_string_set_parameter(frame, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length, button, NULL);

    strcat(grid_lua_state.stdo, frame);

    return 1;
}


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



/*static*/ int l_grid_midi_send(lua_State* L) {

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


    char midiframe[20] = {0};

    sprintf(midiframe, GRID_CLASS_MIDI_frame);

    grid_msg_string_set_parameter(midiframe, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, channel, NULL);
    grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, command, NULL);
    grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, param1, NULL);
    grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, param2, NULL);
    
    //grid_platform_printf("MIDI: %s\r\n", midiframe);  
    strcat(grid_lua_state.stdo, midiframe);

    return 1;
}


/*static*/ int l_grid_midi_sysex_send(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs<2){
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }

    char midiframe[500] = {0};

    sprintf(midiframe, GRID_CLASS_MIDISYSEX_frame_start);

    grid_msg_string_set_parameter(midiframe, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

    int i;
    for ( i=1; i <= nargs; ++i) {

        grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDISYSEX_PAYLOAD_offset + i*2-2, 2, lua_tointeger(L, i), NULL);
    }

    grid_msg_string_set_parameter(midiframe, GRID_CLASS_MIDISYSEX_LENGTH_offset, GRID_CLASS_MIDISYSEX_LENGTH_length, i-1, NULL);

    sprintf(&midiframe[strlen(midiframe)], GRID_CLASS_MIDISYSEX_frame_end);

    //grid_platform_printf("MIDI: %s\r\n", midiframe);  
    strcat(grid_lua_state.stdo, midiframe);

    return 1;
}





/*static*/ int l_grid_led_layer_min(lua_State* L) {

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

    grid_led_set_layer_min(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}


/*static*/ int l_grid_led_layer_mid(lua_State* L) {

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

    grid_led_set_layer_mid(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}

/*static*/ int l_grid_led_layer_max(lua_State* L) {

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

    grid_led_set_layer_max(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);


    return 0;
}



/*static*/ int l_grid_led_layer_color(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs==5){
        
        uint8_t param[5] = {0};

        for (int i=1; i <= nargs; ++i) {
            param[i-1] = lua_tointeger(L, i);
        }


        grid_led_set_layer_color(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);
    }
    else if(nargs==6){

        uint8_t param[6] = {0};

        for (int i=1; i <= nargs; ++i) {
            if (lua_isnil(L, i)){
                param[i-1] = 0;
            }
            else{
                param[i-1] = lua_tointeger(L, i);
            }
        }

        grid_led_set_layer_color(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);
        if (param[5] != 0){
            grid_led_set_layer_min(&grid_led_state, param[0], param[1], 0,0,0);
        }

    }
    else{
        // error
        strcat(grid_lua_state.stde, "#invalidParams");
        return 0;
    }




    return 0;
}

/*static*/ int l_grid_led_layer_frequency(lua_State* L) {

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

    grid_led_set_layer_frequency(&grid_led_state, param[0], param[1], param[2]);
    return 0;
}

/*static*/ int l_grid_led_layer_shape(lua_State* L) {

    

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


    //grid_platform_printf("Led shape %d %d %d\r\n", param[0], param[1], param[2]);

    grid_led_set_layer_shape(&grid_led_state, param[0], param[1], param[2]);


    return 0;
}



/*static*/ int l_grid_led_layer_timeout(lua_State* L) {

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

    grid_led_set_layer_timeout(&grid_led_state, (uint8_t)param[0], (uint8_t)param[1], param[2]);
    return 0;
}



/*static*/ int l_led_default_red(lua_State* L) {

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
            // grid_platform_printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_get_bank_red(&grid_sys_state);
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_set_bank_red(&grid_sys_state, var);
    }
    
    return 1;
}
/*static*/ int l_led_default_green(lua_State* L) {

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
            // grid_platform_printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_get_bank_gre(&grid_sys_state);
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_set_bank_gre(&grid_sys_state, var);
    }
    
    return 1;
}
/*static*/ int l_led_default_blue(lua_State* L) {

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
            // grid_platform_printf(" %d : NIL ", i);
            if (i==1){
                isgetter = 1;
            }
        }

        param[i-1] = lua_tointeger(L, i);
    }


    if (isgetter){

        int32_t var = grid_sys_get_bank_blu(&grid_sys_state);
        lua_pushinteger(L, var);
    }
    else{

        int32_t var =  param[0];
        grid_sys_set_bank_blu(&grid_sys_state, var);
    }
    
    return 1;
}



/*static*/ int l_grid_version_major(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_MAJOR);
    
    return 1;
}


/*static*/ int l_grid_version_minor(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_MINOR);
    
    return 1;
}

/*static*/ int l_grid_version_patch(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, GRID_PROTOCOL_VERSION_PATCH);
    
    return 1;
}

/*static*/ int l_grid_hwcfg(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_get_hwcfg(&grid_sys_state));
    
    return 1;
}

/*static*/ int l_grid_random(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }


    uint8_t random = grid_platform_get_random_8();

    lua_pushinteger(L, random);
    
    return 1;
}

/*static*/ int l_grid_position_x(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }


    lua_pushinteger(L, grid_sys_get_module_x(&grid_sys_state));
    
    return 1;
}

/*static*/ int l_grid_position_y(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_get_module_y(&grid_sys_state));
    
    return 1;
}

/*static*/ int l_grid_rotation(lua_State* L) {

    int nargs = lua_gettop(L);

    if (nargs!=0){
        // error
        strcat(grid_lua_state.stde, "#GTV.invalidParams");
        return 0;
    }

    lua_pushinteger(L, grid_sys_get_module_rot(&grid_sys_state));
    
    return 1;
}



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
           
    uint8_t page = grid_ui_page_get_activepage(&grid_ui_state);
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
            char response[20] = {0};
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

        grid_ui_element_timer_set(ele, param[1]);

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



/* ====================  MODULE SPECIFIC INITIALIZERS  ====================*/

void grid_lua_ui_init_po16(struct grid_lua_model* mod){

    grid_platform_printf("LUA UI INIT PO16\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_P_META_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} ");

    // initialize 16 potmeter
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=0, 15 do setmetatable("GRID_LUA_KW_ELEMENT_short"[i], potmeter_meta) end");

    grid_lua_gc_try_collect(mod);

    //initialize the system element
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"[16] = {index = 16}");
    grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
    grid_lua_dostring(mod, "setmetatable("GRID_LUA_KW_ELEMENT_short"[16], system_meta)");

}

void grid_lua_ui_init_bu16(struct grid_lua_model* mod){

     grid_platform_printf("LUA UI INIT BU16\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_B_META_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} ");

    // initialize 16 buttons
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=0, 15 do setmetatable("GRID_LUA_KW_ELEMENT_short"[i], button_meta) end");

    grid_lua_gc_try_collect(mod);

    //initialize the system element
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"[16] = {index = 16}");
    grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
    grid_lua_dostring(mod, "setmetatable("GRID_LUA_KW_ELEMENT_short"[16], system_meta)");
}

void grid_lua_ui_init_pbf4(struct grid_lua_model* mod){

     grid_platform_printf("LUA UI INIT PBF4\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_P_META_init);
    grid_lua_dostring(mod, GRID_LUA_B_META_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} ");

    // initialize 8 potmeters and 4 buttons
    grid_lua_dostring(mod, "for i=0, 7  do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=0, 7  do  setmetatable("GRID_LUA_KW_ELEMENT_short"[i], potmeter_meta)  end");

    grid_lua_dostring(mod, "for i=8, 11 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=8, 11 do  setmetatable("GRID_LUA_KW_ELEMENT_short"[i], button_meta)  end");

  
    grid_lua_gc_try_collect(mod);

    //initialize the system element
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"[12] = {index = 12}");
    grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
    grid_lua_dostring(mod, "setmetatable("GRID_LUA_KW_ELEMENT_short"[12], system_meta)");

}

void grid_lua_ui_init_en16(struct grid_lua_model* mod){

    grid_platform_printf("LUA UI INIT EN16\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_E_META_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} ");

    // initialize 16 encoders
    grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=0, 15 do setmetatable("GRID_LUA_KW_ELEMENT_short"[i], encoder_meta) end");


    grid_lua_gc_try_collect(mod);

    //initialize the system element
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"[16] = {index = 16}");
    grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
    grid_lua_dostring(mod, "setmetatable("GRID_LUA_KW_ELEMENT_short"[16], system_meta)");
}

void grid_lua_ui_init_ef44(struct grid_lua_model* mod){

    grid_platform_printf("LUA UI INIT EF44\r\n");
    // define encoder_init_function

    grid_lua_dostring(mod, GRID_LUA_E_META_init);
    grid_lua_dostring(mod, GRID_LUA_P_META_init);

    // create element array
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"= {} ");

    // initialize 4 encoders and 4 faders
    grid_lua_dostring(mod, "for i=0, 3  do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=0, 3  do  setmetatable("GRID_LUA_KW_ELEMENT_short"[i], encoder_meta)  end");

    grid_lua_dostring(mod, "for i=4, 7 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
    grid_lua_dostring(mod, "for i=4, 7 do  setmetatable("GRID_LUA_KW_ELEMENT_short"[i], potmeter_meta)  end");

  
    grid_lua_gc_try_collect(mod);

    //initialize the system element
    grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short"[8] = {index = 8}");
    grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
    grid_lua_dostring(mod, "setmetatable("GRID_LUA_KW_ELEMENT_short"[8], system_meta)");

}



void grid_lua_ui_init(struct grid_lua_model* mod, struct grid_sys_model* sys){


    grid_platform_printf("LUA UI INIT HWCFG:%d\r\n", grid_sys_get_hwcfg(sys));

    //register init functions for different ui elements

    switch (grid_sys_get_hwcfg(sys)){

        case GRID_MODULE_PO16_RevB: grid_lua_ui_init_po16(mod); break;
        case GRID_MODULE_PO16_RevC: grid_lua_ui_init_po16(mod); break;
        case GRID_MODULE_PO16_RevD: grid_lua_ui_init_po16(mod); break;

        case GRID_MODULE_BU16_RevB: grid_lua_ui_init_bu16(mod); break;
        case GRID_MODULE_BU16_RevC: grid_lua_ui_init_bu16(mod); break;
        case GRID_MODULE_BU16_RevD: grid_lua_ui_init_bu16(mod); break;

        case GRID_MODULE_PBF4_RevA: grid_lua_ui_init_pbf4(mod); break;
        case GRID_MODULE_PBF4_RevD: grid_lua_ui_init_pbf4(mod); break;

        case GRID_MODULE_EN16_RevA: grid_lua_ui_init_en16(mod); break;
        case GRID_MODULE_EN16_RevD: grid_lua_ui_init_en16(mod); break;

        case GRID_MODULE_EN16_ND_RevA: grid_lua_ui_init_en16(mod); break;
        case GRID_MODULE_EN16_ND_RevD: grid_lua_ui_init_en16(mod); break;

        case GRID_MODULE_EF44_RevA: grid_lua_ui_init_ef44(mod); break;
        case GRID_MODULE_EF44_RevD: grid_lua_ui_init_ef44(mod); break;

        default: grid_platform_printf("\r\n### LUA HWCFG NOT REGISTERED ### \r\n\r\n");

    }

}


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


void grid_lua_stop_vm(struct grid_lua_model* mod){

    if (mod->L != NULL){

        lua_close(mod->L);
        mod->L = NULL;

    }
}


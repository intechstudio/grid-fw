/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 */ 

#include "grid_usb.h"


struct grid_midi_event_desc grid_midi_tx_buffer[GRID_MIDI_TX_BUFFER_length];

uint16_t grid_midi_tx_write_index;
uint16_t grid_midi_tx_read_index;

struct grid_midi_event_desc grid_midi_rx_buffer[GRID_MIDI_RX_BUFFER_length];
uint16_t grid_midi_rx_write_index;
uint16_t grid_midi_rx_read_index;


struct grid_usb_keyboard_model grid_usb_keyboard_state;

void grid_usb_midi_buffer_init()
{
	grid_midi_tx_write_index = 0;
	grid_midi_tx_read_index = 0;
	grid_midi_buffer_init(grid_midi_tx_buffer, GRID_MIDI_TX_BUFFER_length);

	grid_midi_rx_write_index = 0;
	grid_midi_rx_read_index = 0;
	grid_midi_buffer_init(grid_midi_rx_buffer, GRID_MIDI_RX_BUFFER_length);
		


}

void grid_usb_keyboard_model_init(struct grid_usb_keyboard_model* kb, uint8_t buffer_length){
    
	kb->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
	kb->tx_write_index = 0;
	kb->tx_read_index = 0;


	kb->tx_buffer_length = buffer_length;

	kb->tx_buffer = (struct grid_usb_keyboard_event_desc*) malloc(buffer_length*sizeof(struct grid_usb_keyboard_event_desc));

	for (uint16_t i=0; i<kb->tx_buffer_length; i++)
	{
		kb->tx_buffer[i].ismodifier = 0;
		kb->tx_buffer[i].keycode = 0;
		kb->tx_buffer[i].ispressed = 0;
		kb->tx_buffer[i].delay = 0;
	}

    
	
	for (uint8_t i=0; i<GRID_KEYBOARD_KEY_maxcount; i++)
	{

		
		
		kb->active_key_list[i].ismodifier = 0;
		kb->active_key_list[i].keycode = 255;
		kb->active_key_list[i].ispressed = 0;
		
	}
	
	kb->active_key_count = 0;
    
    kb->isenabled = 1;
	
}

uint8_t grid_usb_keyboard_cleanup(struct grid_usb_keyboard_model* kb){
	
	uint8_t changed_flag = 0;
	
	// Remove all inactive (released) keys
	for(uint8_t i=0; i<kb->active_key_count; i++){
		
		if (kb->active_key_list[i].ispressed == false){
			
			changed_flag = 1;
			
			kb->active_key_list[i].ismodifier = 0;
			kb->active_key_list[i].ispressed = 0;
			kb->active_key_list[i].keycode = 255;	
					
			// Pop item, move each remaining after this forvard one index
			for (uint8_t j=i+1; j<kb->active_key_count; j++){
				
				kb->active_key_list[j-1] = kb->active_key_list[j];
				
				kb->active_key_list[j].ismodifier = 0;
				kb->active_key_list[j].ispressed = 0;
				kb->active_key_list[j].keycode = 255;
				
			}
			
			kb->active_key_count--;
			i--; // Retest this index, because it now points to a new item
		}
		
	}
	
	if (changed_flag == 1){
			
//		uint8_t debugtext[100] = {0};
//		snprintf(debugtext, 99, "count: %d | activekeys: %d, %d, %d, %d, %d, %d", kb->active_key_count, kb->active_key_list[0].keycode, kb->active_key_list[1].keycode, kb->active_key_list[2].keycode, kb->active_key_list[3].keycode, kb->active_key_list[4].keycode, kb->active_key_list[5].keycode);
//		grid_port_debug_print_text(debugtext);
			
			
		// USB SEND
	}
	
	return changed_flag;
	
}


extern int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);


void grid_usb_keyboard_keychange(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc* key){
	
	uint8_t item_index = 255;
	uint8_t changed_flag = 0;
	

	grid_usb_keyboard_cleanup(kb);
	

	for(uint8_t i=0; i<kb->active_key_count; i++){
		
		if (kb->active_key_list[i].keycode == key->keycode && kb->active_key_list[i].ismodifier == key->ismodifier){
			// key is already in the list
			item_index = i;
			
			if (kb->active_key_list[i].ispressed == true){
				
				if (key->ispressed == true){
					// OK nothing to do here
				}
				else{
					// Release the damn key
					kb->active_key_list[i].ispressed = false;
					changed_flag = 1;
				}
				
			}
			
		}
		
	}
	
	
	grid_usb_keyboard_cleanup(kb);
	
	
	if (item_index == 255){
		
		// item not in list
		
		if (kb->active_key_count< GRID_KEYBOARD_KEY_maxcount){
			
			if (key->ispressed == true){
				
				kb->active_key_list[kb->active_key_count] = *key;
				kb->active_key_count++;
				changed_flag = 1;
				
			}
		
		}
		else{
			//grid_port_debug_print_text("activekeys limit hit!");
		}
		
	}
	
	
	if (changed_flag == 1){
		
        if (kb->isenabled){
            

			

    		grid_platform_usb_keyboard_keys_state_change(kb->active_key_list, kb->active_key_count);    
        
        }
        else{
        
            grid_port_debug_print_text("KB IS DISABLED");
                   
            // Generate ACKNOWLEDGE RESPONSE
            struct grid_msg_packet message;

            grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

			grid_msg_packet_body_append_printf(&message, GRID_CLASS_HIDKEYSTATUS_frame);
			grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length, kb->isenabled);
			
            grid_msg_packet_close(&grid_msg_state, &message);
            grid_port_packet_send_everywhere(&message);
            
        }

		// USB SEND
	}
	
}



void grid_midi_buffer_init(struct grid_midi_event_desc* buf, uint16_t length){
	
	
	for (uint16_t i=0; i<length; i++)
	{
		buf[i].byte0 = 0;
		buf[i].byte1 = 0;
		buf[i].byte2 = 0;
		buf[i].byte3 = 0;
	}
	
}

uint8_t grid_midi_tx_push(struct grid_midi_event_desc midi_event){

	grid_midi_tx_buffer[grid_midi_tx_write_index] = midi_event;
	grid_midi_tx_write_index = (grid_midi_tx_write_index+1)%GRID_MIDI_TX_BUFFER_length;

	uint32_t space_in_buffer = (grid_midi_tx_read_index-grid_midi_tx_write_index + GRID_MIDI_TX_BUFFER_length)%GRID_MIDI_TX_BUFFER_length;

	uint8_t return_packet_was_dropped = 0;

	if (space_in_buffer == 0){
		return_packet_was_dropped = 1;
		//Increment teh read index to drop latest packet and make space for a new one.
		grid_midi_tx_read_index = (grid_midi_tx_read_index+1)%GRID_MIDI_TX_BUFFER_length;
	}

	//printf("W: %d %d : %d\r\n", grid_midi_tx_write_index, grid_midi_tx_read_index, space_in_buffer);

	return return_packet_was_dropped;


}

void grid_midi_tx_pop(){

	if (grid_midi_tx_read_index != grid_midi_tx_write_index){
		
		if (grid_platform_usb_midi_write_status() != 1){

			uint8_t byte0 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte0;
			uint8_t byte1 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte1;
			uint8_t byte2 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte2;
			uint8_t byte3 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte3;
			
			grid_midi_tx_read_index = (grid_midi_tx_read_index+1)%GRID_MIDI_TX_BUFFER_length;
			grid_platform_usb_midi_write(byte0, byte1, byte2, byte3);



		}
		
	}

}


void grid_midi_rx_push(struct grid_midi_event_desc midi_event){

	// MIDI RX IS DISABLED


	if (grid_sys_get_midirx_any_state(&grid_sys_state) == 0){
		return;
	}

	if (grid_sys_get_midirx_sync_state(&grid_sys_state) == 0){

		if (midi_event.byte0 == 8 && midi_event.byte1 == 240){
			// midi clock message was recieved
			return;

		}

		if (midi_event.byte0 == 10 && midi_event.byte1 == 240){
			// midi start message was recieved
			return;

		}

		if (midi_event.byte0 == 12 && midi_event.byte1 == 240){
			// midi stop message was recieved
			return;

		}

	}

	

	// recude commandchange time resolution HERE!!

	//       W              R
	//[0][1][2][3][4][5][6][7][8][9][10]

	//grid_port_debug_printf("PUSH: %d %d", grid_midi_rx_write_index, grid_midi_rx_read_index);

	for(uint16_t i=0; i<GRID_MIDI_RX_BUFFER_length; i++){

		if (grid_midi_rx_write_index-i == grid_midi_rx_read_index){

			grid_midi_rx_buffer[grid_midi_rx_write_index] = midi_event;
			grid_midi_rx_write_index = (grid_midi_rx_write_index+1)%GRID_MIDI_RX_BUFFER_length;
			break; //return

		}

		if (grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte2 != GRID_PARAMETER_MIDI_CONTROLCHANGE) continue;

		if (grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte0 != midi_event.byte0) continue;
		if (grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte1 != midi_event.byte1) continue;
		if (grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte2 != midi_event.byte2) continue;

		// it's a match, update to the newer value!!
		//grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);
		grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte3 = midi_event.byte3;

		break; //return;

	}


}

void grid_midi_rx_pop(){



	if (grid_midi_rx_read_index != grid_midi_rx_write_index){


		//grid_port_debug_printf("POP: %d %d", grid_midi_rx_write_index, grid_midi_rx_read_index);
			
		// Combine multiple midi messages into one packet if possible

		// SX SY Global, DX DY Global
		struct grid_msg_packet message;
		grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);


		// combine up to 6 midi messages into a packet
		for (uint8_t i = 0; i<6; i++){

			if(grid_midi_rx_read_index != grid_midi_rx_write_index){

				uint8_t byte0 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte0;
				uint8_t byte1 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte1;
				uint8_t byte2 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte2;
				uint8_t byte3 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte3;

				grid_msg_packet_body_append_printf(&message, GRID_CLASS_MIDI_frame);
				grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
				
				grid_msg_packet_body_append_parameter(&message, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, byte0);
				grid_msg_packet_body_append_parameter(&message, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, byte1);
				grid_msg_packet_body_append_parameter(&message, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, byte2);
				grid_msg_packet_body_append_parameter(&message, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, byte3);

				grid_midi_rx_read_index = (grid_midi_rx_read_index+1)%GRID_MIDI_RX_BUFFER_length;
			}

		}

		grid_msg_packet_close(&grid_msg_state, &message);
		grid_port_packet_send_everywhere(&message);

		
	}
	else{

		//no message in fifo
	}

}


uint8_t grid_usb_keyboard_tx_push(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc keyboard_event){

	//printf("R: %d, W: %d\r\n", grid_midi_tx_read_index, grid_midi_tx_write_index);
	//printf("kb tx R: %d, W: %d\r\n", kb->tx_read_index, kb->tx_write_index);


	kb->tx_buffer[kb->tx_write_index] = keyboard_event;

	kb->tx_write_index = (kb->tx_write_index+1)%kb->tx_buffer_length;

	uint32_t space_in_buffer = (kb->tx_read_index-kb->tx_write_index + kb->tx_buffer_length)%kb->tx_buffer_length;

	uint8_t return_packet_was_dropped = 0;

	if (space_in_buffer == 0){
		return_packet_was_dropped = 1;
		//Increment teh read index to drop latest packet and make space for a new one.
		kb->tx_read_index = (kb->tx_read_index+1)%kb->tx_buffer_length;
	}

	//printf("W: %d %d : %d\r\n", kb->tx_write_index, kb->tx_read_index, space_in_buffer);

	return return_packet_was_dropped;



}

void grid_usb_keyboard_tx_pop(struct grid_usb_keyboard_model* kb){

	if (kb->tx_read_index != kb->tx_write_index){
		
        
        
        uint32_t elapsed = grid_platform_rtc_get_elapsed_time(kb->tx_rtc_lasttimestamp);
        
        
		if (elapsed > kb->tx_buffer[kb->tx_read_index].delay*MS_TO_US){
			

            struct grid_usb_keyboard_event_desc key;
            
            key.ismodifier = kb->tx_buffer[kb->tx_read_index].ismodifier;
            key.keycode =    kb->tx_buffer[kb->tx_read_index].keycode;
            key.ispressed =  kb->tx_buffer[kb->tx_read_index].ispressed;
            key.delay = 0;

			kb->tx_read_index = (kb->tx_read_index+1)%kb->tx_buffer_length;
 
			kb->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();

			// 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay

			if (key.ismodifier == 0 || key.ismodifier == 1){

           		grid_usb_keyboard_keychange(&grid_usb_keyboard_state, &key);
			}
			else if(key.ismodifier == 2){ // mousemove

				uint8_t axis = key.keycode; 
				int8_t position = key.ispressed - 128;
				grid_platform_usb_mouse_move(position, axis);


				// grid_port_debug_printf("MouseMove: %d %d", position, axis);	

			}
			else if(key.ismodifier == 3){

										
				uint8_t state = key.ispressed;
				uint8_t button = key.keycode;
				grid_platform_usb_mouse_button_change(state, button);
			
				// grid_port_debug_printf("MouseButton: %d %d", state, button);	
				

			}
			else if(key.ismodifier == 0xf){
				// delay, nothing to do here
			}
			else{
				//printf("Keyboard Mouse Invalid\r\n");	
			}


		}
		
	}

}

void grid_usb_gamepad_axis_move(uint8_t axis, int32_t move){

	grid_platform_usb_gamepad_axis_move(axis, move);
}

void grid_usb_gamepad_button_change(uint8_t button, uint8_t value){

	grid_platform_usb_gamepad_button_change(button, value);
}


void grid_usb_keyboard_enable(struct grid_usb_keyboard_model* kb){

	kb->isenabled = 1;

}

void grid_usb_keyboard_disable(struct grid_usb_keyboard_model* kb){

	kb->isenabled = 0;
	
}


uint8_t grid_usb_keyboard_isenabled(struct grid_usb_keyboard_model* kb){

	return kb->isenabled;

}


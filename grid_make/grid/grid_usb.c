/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 */ 

#include "grid_usb.h"
#include "../usb/class/midi/device/audiodf_midi.h"

volatile uint16_t grid_usb_rx_double_buffer_index;
volatile uint8_t grid_usb_serial_rx_buffer[CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ];

static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{



	//printf("\r\n$ %d ", count);


	uint8_t halfpacket = 0;
	if (grid_usb_serial_rx_buffer[0] != GRID_CONST_SOH || grid_usb_serial_rx_buffer[count-1] != 10){
		halfpacket = 1;
		//printf("halfpacket");
	}

	for(uint16_t i=0; i<count; i++){

		// if (halfpacket && (i<10 || i>count-10)){
		// 	printf(" %02x", grid_usb_serial_rx_buffer[i]);
		// }
		// else if (halfpacket && i == 10){
		// 	printf(" ...");
		// }

		GRID_PORT_H.rx_double_buffer[grid_usb_rx_double_buffer_index] = grid_usb_serial_rx_buffer[i];

		//printf("%d, ", grid_usb_serial_rx_buffer[i]);
		
		grid_usb_rx_double_buffer_index++;
		grid_usb_rx_double_buffer_index%=GRID_DOUBLE_BUFFER_RX_SIZE;

	}

	// CLEAR THE ENTIRE BUFFER
	for(uint16_t i=0; i<sizeof(grid_usb_serial_rx_buffer); i++){
		grid_usb_serial_rx_buffer[i] = 0;
	}


	cdcdf_acm_read((uint8_t *)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));

	//cdcdf_acm_write(cdcdf_demo_buf, count); /* Echo data */
	return false;                           /* No error. */
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{

	//grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);

	
	return false;                                                                                 /* No error. */
}
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state)
{
	
	//grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	
	//printf("\r\n### USB SERIAL STATE CHANGE %d ###\r\n", sizeof(grid_usb_serial_rx_buffer));

	if (state.rs232.DTR || 1) {
		/* After connection the R/W callbacks can be registered */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_serial_bulkout_cb);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_serial_bulkin_cb);
		/* Start Rx */
		cdcdf_acm_read((uint8_t *)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));
	}

	return false; /* No error. */
}
void grid_usb_serial_init()
{

	grid_usb_serial_rx_size = 0;
	grid_usb_serial_rx_flag = 0;
	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)grid_usb_serial_statechange_cb);
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkout_cb);
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);
}



static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{

	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	return false;
}




void grid_usb_midi_init()
{
	grid_midi_tx_write_index = 0;
	grid_midi_tx_read_index = 0;
	grid_midi_buffer_init(grid_midi_tx_buffer, GRID_MIDI_TX_BUFFER_length);

	grid_midi_rx_write_index = 0;
	grid_midi_rx_read_index = 0;
	grid_midi_buffer_init(grid_midi_rx_buffer, GRID_MIDI_RX_BUFFER_length);
		
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkin_cb);


}

void grid_keyboard_init(struct grid_keyboard_model* kb){
    
	grid_keyboard_tx_rtc_lasttimestamp = grid_sys_rtc_get_time(&grid_sys_state);
	grid_keyboard_tx_write_index = 0;
	grid_keyboard_tx_read_index = 0;
	grid_keyboard_buffer_init(grid_keyboard_tx_buffer, GRID_KEYBOARD_TX_BUFFER_length);
    
    
	
	for (uint8_t i=0; i<GRID_KEYBOARD_KEY_maxcount; i++)
	{
		kb->hid_key_array[i].b_modifier = false;
		kb->hid_key_array[i].key_id = 255;
		kb->hid_key_array[i].state = HID_KB_KEY_UP;
		
		
		kb->key_list[i].ismodifier = 0;
		kb->key_list[i].ispressed = 0;
		kb->key_list[i].keycode = 255;
		
	}
	
	kb->key_active_count = 0;
    
    kb->isenabled = 1;
	
}

uint8_t grid_keyboard_cleanup(struct grid_keyboard_model* kb){
	
	uint8_t changed_flag = 0;
	
	// Remove all inactive (released) keys
	for(uint8_t i=0; i<kb->key_active_count; i++){
		
		if (kb->key_list[i].ispressed == false){
			
			changed_flag = 1;
			
			kb->key_list[i].ismodifier = 0;
			kb->key_list[i].ispressed = 0;
			kb->key_list[i].keycode = 255;	
					
			// Pop item, move each remaining after this forvard one index
			for (uint8_t j=i+1; j<kb->key_active_count; j++){
				
				kb->key_list[j-1] = kb->key_list[j];
				
				kb->key_list[j].ismodifier = 0;
				kb->key_list[j].ispressed = 0;
				kb->key_list[j].keycode = 255;
				
			}
			
			kb->key_active_count--;
			i--; // Retest this index, because it now points to a new item
		}
		
	}
	
	if (changed_flag == 1){
			
//		uint8_t debugtext[100] = {0};
//		snprintf(debugtext, 99, "count: %d | activekeys: %d, %d, %d, %d, %d, %d", kb->key_active_count, kb->key_list[0].keycode, kb->key_list[1].keycode, kb->key_list[2].keycode, kb->key_list[3].keycode, kb->key_list[4].keycode, kb->key_list[5].keycode);
//		grid_debug_print_text(debugtext);
			
			
		// USB SEND
	}
	
	return changed_flag;
	
}


uint8_t grid_keyboard_keychange(struct grid_keyboard_model* kb, struct grid_keyboard_event_desc* key){
	
	uint8_t item_index = 255;
	uint8_t remove_flag = 0;
	uint8_t changed_flag = 0;
	

	grid_keyboard_cleanup(kb);
	

	for(uint8_t i=0; i<kb->key_active_count; i++){
		
		if (kb->key_list[i].keycode == key->keycode && kb->key_list[i].ismodifier == key->ismodifier){
			// key is already in the list
			item_index = i;
			
			if (kb->key_list[i].ispressed == true){
				
				if (key->ispressed == true){
					// OK nothing to do here
				}
				else{
					// Release the damn key
					kb->key_list[i].ispressed = false;
					changed_flag = 1;
				}
				
			}
			
		}
		
	}
	
	
	uint8_t print_happened = grid_keyboard_cleanup(kb);
	
	
	if (item_index == 255){
		
		// item not in list
		
		if (kb->key_active_count< GRID_KEYBOARD_KEY_maxcount){
			
			if (key->ispressed == true){
				
				kb->key_list[kb->key_active_count] = *key;
				kb->key_active_count++;
				changed_flag = 1;
				
			}
		
		}
		else{
			//grid_debug_print_text("activekeys limit hit!");
		}
		
	}
	
	
	if (changed_flag == 1){
		
//		uint8_t debugtext[100] = {0};
//		snprintf(debugtext, 99, "cound: %d | activekeys: %d, %d, %d, %d, %d, %d", kb->key_active_count, kb->key_list[0].keycode, kb->key_list[1].keycode, kb->key_list[2].keycode, kb->key_list[3].keycode, kb->key_list[4].keycode, kb->key_list[5].keycode);	
//		
//		if (!print_happened){
//			
//			
//			grid_debug_print_text(debugtext);
//		}
			
		
		for(uint8_t i=0; i<GRID_KEYBOARD_KEY_maxcount; i++){
		
			kb->hid_key_array[i].b_modifier = kb->key_list[i].ismodifier;
			kb->hid_key_array[i].key_id = kb->key_list[i].keycode;
			kb->hid_key_array[i].state = kb->key_list[i].ispressed;
		
		}
        
        
        if (kb->isenabled){
            
    		hiddf_keyboard_keys_state_change(kb->hid_key_array, kb->key_active_count);    
        
        }
        else{
        
            grid_debug_print_text("KB IS DISABLED");
                   
            // Generate ACKNOWLEDGE RESPONSE
            struct grid_msg message;

            grid_msg_init_header(&grid_msg_state, &message, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

						grid_msg_body_append_printf(&message, GRID_CLASS_HIDKEYSTATUS_frame);
						grid_msg_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
						grid_msg_body_append_parameter(&message, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length, kb->isenabled);
						
            grid_msg_packet_close(&grid_msg_state, &message);
            grid_sys_packet_send_everywhere(&message);
            
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

uint8_t grid_midi_tx_pop(){

	if (grid_midi_tx_read_index != grid_midi_tx_write_index){
		
		if (audiodf_midi_write_status() != USB_BUSY){

			uint8_t byte0 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte0;
			uint8_t byte1 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte1;
			uint8_t byte2 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte2;
			uint8_t byte3 = grid_midi_tx_buffer[grid_midi_tx_read_index].byte3;
			
			grid_midi_tx_read_index = (grid_midi_tx_read_index+1)%GRID_MIDI_TX_BUFFER_length;
			uint32_t space_in_buffer = (grid_midi_tx_read_index-grid_midi_tx_write_index + GRID_MIDI_TX_BUFFER_length)%GRID_MIDI_TX_BUFFER_length;
	

			//printf("R: %d %d: %d\r\n", grid_midi_tx_write_index, grid_midi_tx_read_index, space_in_buffer);
			audiodf_midi_write(byte0, byte1, byte2, byte3);



		}
		
	}

}


uint8_t grid_midi_rx_push(struct grid_midi_event_desc midi_event){

	// MIDI RX IS DISABLED


	if (grid_sys_get_midirx_any_state(&grid_sys_state) == 0){
		return;
	}

	if (grid_sys_get_midirx_sync_state(&grid_sys_state) == 0){

		if (midi_event.byte0 == 8 && midi_event.byte1 == 240 && midi_event.byte2 == 36){
			// midi clock message was recieved
			return;

		}

		if (midi_event.byte0 == 10 && midi_event.byte1 == 240 && midi_event.byte2 == 36){
			// midi start message was recieved
			return;

		}

		if (midi_event.byte0 == 12 && midi_event.byte1 == 240 && midi_event.byte2 == 36){
			// midi stop message was recieved
			return;

		}

	}

	// recude commandchange time resolution HERE!!

	//       W              R
	//[0][1][2][3][4][5][6][7][8][9][10]


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
		grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);
		grid_midi_rx_buffer[grid_midi_rx_write_index-i].byte3 = midi_event.byte3;

		break; //return;

	}


}

uint8_t grid_midi_rx_pop(){

	if (grid_midi_rx_read_index != grid_midi_rx_write_index){
		

		uint8_t byte0 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte0;
		uint8_t byte1 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte1;
		uint8_t byte2 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte2;
		uint8_t byte3 = grid_midi_rx_buffer[grid_midi_rx_read_index].byte3;
			
		// Combine multiple midi messages into one packet if possible

		// SX SY Global, DX DY Global
		struct grid_msg message;
		grid_msg_init_header(&grid_msg_state, &message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION);
		grid_msg_header_set_sx(&message, GRID_SYS_DEFAULT_POSITION);
		grid_msg_header_set_sy(&message, GRID_SYS_DEFAULT_POSITION);

		// combuine up to 6 midi messages into a packet
		for (uint8_t i = 0; i<6; i++){

			if(grid_midi_rx_read_index != grid_midi_rx_write_index){

				grid_msg_body_append_printf(&message, GRID_CLASS_MIDI_frame);
				grid_msg_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
				
				grid_msg_body_append_parameter(&message, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, byte0);
				grid_msg_body_append_parameter(&message, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, byte1);
				grid_msg_body_append_parameter(&message, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, byte2);
				grid_msg_body_append_parameter(&message, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, byte3);

				grid_midi_rx_read_index = (grid_midi_rx_read_index+1)%GRID_MIDI_RX_BUFFER_length;
			}

		}

		grid_msg_packet_close(&grid_msg_state, &message);
		grid_sys_packet_send_everywhere(&message);

		
	}

}


void grid_keyboard_buffer_init(struct grid_keyboard_event_desc* buf, uint16_t length){
	
	
	for (uint16_t i=0; i<length; i++)
	{
		buf[i].ismodifier = 0;
		buf[i].keycode = 0;
		buf[i].ispressed = 0;
		buf[i].delay = 0;
	}
	
}

uint8_t grid_keyboard_tx_push(struct grid_keyboard_event_desc keyboard_event){

	//printf("R: %d, W: %d\r\n", grid_midi_tx_read_index, grid_midi_tx_write_index);
	//printf("kb tx R: %d, W: %d\r\n", grid_keyboard_tx_read_index, grid_keyboard_tx_write_index);


	grid_keyboard_tx_buffer[grid_keyboard_tx_write_index] = keyboard_event;

	grid_keyboard_tx_write_index = (grid_keyboard_tx_write_index+1)%GRID_KEYBOARD_TX_BUFFER_length;

	uint32_t space_in_buffer = (grid_keyboard_tx_read_index-grid_keyboard_tx_write_index + GRID_KEYBOARD_TX_BUFFER_length)%GRID_KEYBOARD_TX_BUFFER_length;

	uint8_t return_packet_was_dropped = 0;

	if (space_in_buffer == 0){
		return_packet_was_dropped = 1;
		//Increment teh read index to drop latest packet and make space for a new one.
		grid_keyboard_tx_read_index = (grid_keyboard_tx_read_index+1)%GRID_KEYBOARD_TX_BUFFER_length;
	}

	//printf("W: %d %d : %d\r\n", grid_keyboard_tx_write_index, grid_keyboard_tx_read_index, space_in_buffer);

	return return_packet_was_dropped;



}

uint8_t grid_keyboard_tx_pop(){

	if (grid_keyboard_tx_read_index != grid_keyboard_tx_write_index){
		
        
        
        uint32_t elapsed = grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_keyboard_tx_rtc_lasttimestamp);
        
        
		if (elapsed > grid_keyboard_tx_buffer[grid_keyboard_tx_read_index].delay*RTC1MS){
			
			uint32_t space_in_buffer = (grid_keyboard_tx_read_index-grid_keyboard_tx_write_index + GRID_KEYBOARD_TX_BUFFER_length)%GRID_KEYBOARD_TX_BUFFER_length;

			//printf("R: %d %d : %d\r\n", grid_keyboard_tx_write_index, grid_keyboard_tx_read_index, space_in_buffer);
            
            struct grid_keyboard_event_desc key;
            
            key.ismodifier = grid_keyboard_tx_buffer[grid_keyboard_tx_read_index].ismodifier;
            key.keycode =    grid_keyboard_tx_buffer[grid_keyboard_tx_read_index].keycode;
            key.ispressed =  grid_keyboard_tx_buffer[grid_keyboard_tx_read_index].ispressed;
            key.delay = 0;

			grid_keyboard_tx_read_index = (grid_keyboard_tx_read_index+1)%GRID_KEYBOARD_TX_BUFFER_length;
            
            grid_keyboard_tx_rtc_lasttimestamp = grid_sys_rtc_get_time(&grid_sys_state);

			// 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay

			if (key.ismodifier == 0 || key.ismodifier == 1){

           		grid_keyboard_keychange(&grid_keyboard_state, &key);
			}
			else if(key.ismodifier == 2){ // mousemove

				uint8_t axis = key.keycode; 
				int8_t position = key.ispressed - 128;
				hiddf_mouse_move(position, axis);

				// grid_debug_printf("MouseMove: %d %d", position, axis);	

			}
			else if(key.ismodifier == 3){

										
				uint8_t state = key.ispressed;
				uint8_t button = key.keycode;
				hiddf_mouse_button_change(state, button);
			
				// grid_debug_printf("MouseButton: %d %d", state, button);	
				

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


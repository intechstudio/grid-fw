#include "grid_module_en16_reva.h"



struct grid_ui_encoder grid_ui_encoder_array[16];

uint8_t UI_SPI_TX_BUFFER[14] = "aaaaaaaaaaaaaa";
uint8_t UI_SPI_RX_BUFFER[14];
uint8_t UI_SPI_TRANSFER_LENGTH = 10;

volatile uint8_t UI_SPI_DONE = 0;


volatile uint8_t UI_SPI_RX_BUFFER_LAST[16];

uint8_t UI_ENCODER_BUTTON_STATE[16];
uint8_t UI_ENCODER_BUTTON_STATE_CHANGED[16];

uint8_t UI_ENCODER_ROTATION_STATE[16];
uint8_t UI_ENCODER_ROTATION_STATE_CHANGED[16];


uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;










void grid_module_en16_reva_hardware_start_transfer(void){
	

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);

	spi_m_async_enable(&UI_SPI);

	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

void grid_module_en16_reva_hardware_transfer_complete_cb(void){

	/* Transfer completed */

	struct grid_ui_model* mod = &grid_ui_state;
	
	
	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

		
	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t j=0; j<16; j++){
		

		uint8_t i = UI_ENCODER_LOOKUP[j];
		

		uint8_t new_value = (UI_SPI_RX_BUFFER[j/2]>>(4*(j%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];
			
		if (old_value != new_value){

				
			UI_SPI_DEBUG = j;
				
			uint8_t button_value = new_value>>2;
			uint8_t phase_a = (new_value>>1)&1;
			uint8_t phase_b = (new_value)&1;
				
			if (button_value != grid_ui_encoder_array[i].button_value){
				// BUTTON CHANGE
				grid_ui_encoder_array[i].button_changed = 1;
				grid_ui_encoder_array[i].button_value = new_value>>2;
				
				
				//CRITICAL_SECTION_ENTER()
					
				uint8_t command;
				uint8_t velocity;
				
				if (grid_ui_encoder_array[i].button_value == 0){
					
					command = GRID_MSG_COMMAND_MIDI_NOTEON;
					velocity = 127;
				}
				else{
					
					command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
					velocity = 0;
				}
				
				uint8_t actuator = 2*velocity;
				
				grid_sys_write_hex_string_value(&mod->report_ui_array[i].payload[5], 2, command);
				grid_sys_write_hex_string_value(&mod->report_ui_array[i].payload[7], 2, i);
				grid_sys_write_hex_string_value(&mod->report_ui_array[i].payload[9], 2, velocity);
				
				//grid_sys_write_hex_string_value(&mod->report_ui_array[i].payload[21], 2, actuator);
				mod->report_ui_array[i].helper[0] = velocity;
				
				grid_report_ui_set_changed_flag(mod, i);
				
				
				//CRITICAL_SECTION_LEAVE()
				
				
			}
				
			uint8_t a_now = phase_a;
			uint8_t b_now = phase_b;
			
			uint8_t a_prev = grid_ui_encoder_array[i].phase_a_previous;
			uint8_t b_prev = grid_ui_encoder_array[i].phase_b_previous;
			
			int8_t delta = 0;
			
			if (a_now != a_prev){
				
				if (b_now != a_now){
					delta = -1;
				}
				else{
					delta = +1;
				}
				
				
			}
			

			
			grid_ui_encoder_array[i].phase_a_previous = a_now;
			grid_ui_encoder_array[i].phase_b_previous = b_now;
						
			if (delta != 0){
				
				volatile uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_ui_encoder_array[i+1].last_real_time);
				
				if (elapsed_time>400){
					elapsed_time = 400;
				}
				
				if (elapsed_time<20){
					elapsed_time = 20;
				}
			
				
				uint8_t velocityfactor = (160000-elapsed_time*elapsed_time)/40000.0 + 1;
				
				
				grid_ui_encoder_array[i].last_real_time = grid_sys_rtc_get_time(&grid_sys_state);
				
				int16_t xi = delta + delta * velocityfactor;
				
				if (delta<0){
					if (grid_ui_encoder_array[i].rotation_value + xi >= 0){
						grid_ui_encoder_array[i].rotation_value += xi;
					}
					else{
						grid_ui_encoder_array[i].rotation_value = 0;
					}
				}
				else if (delta>0){
					if (grid_ui_encoder_array[i].rotation_value + xi <= 127){
						grid_ui_encoder_array[i].rotation_value += xi;
					}
					else{
						grid_ui_encoder_array[i].rotation_value = 127;
					}
				}
				

				//CRITICAL_SECTION_ENTER()
				
				uint8_t command = GRID_MSG_COMMAND_MIDI_CONTROLCHANGE;
				
				
				uint8_t value = 0;
				if (0 == grid_report_ui_get_changed_flag(mod, i+16)){
					value = 64; //CENTER
					mod->report_ui_array[i+16].helper[0] = 0;
				}
				else{
					value = mod->report_ui_array[i+16].helper[0];
				}
				
				value +=  delta*velocityfactor;
				
				uint8_t actuator = 2*grid_ui_encoder_array[i].rotation_value;
				
				if (value != mod->report_ui_array[i+16].helper[0]){
					
					grid_sys_write_hex_string_value(&mod->report_ui_array[i+16].payload[5], 2, command);
					grid_sys_write_hex_string_value(&mod->report_ui_array[i+16].payload[7], 2, i);
					grid_sys_write_hex_string_value(&mod->report_ui_array[i+16].payload[9], 2, value);
					
					mod->report_ui_array[i+16].helper[0] = value;
					grid_report_ui_set_changed_flag(mod, i+16);
					
					
					
					grid_sys_write_hex_string_value(&mod->report_ui_array[i+16+16].payload[9], 2, actuator); // LED
					mod->report_ui_array[i+16+16].helper[0] = actuator;
					grid_report_ui_set_changed_flag(mod, i+16+16);
					
				}
				
			}			
			else{ //DELTA==0

				if (grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_ui_encoder_array[i].last_real_time)>200){
					if (grid_ui_encoder_array[i].rotation_value > 64){

						grid_ui_encoder_array[i].rotation_value--;
						uint8_t v = 2 * grid_ui_encoder_array[i].rotation_value;

						grid_sys_write_hex_string_value(&mod->report_ui_array[i+16+16].payload[9], 2, v); // LED
						mod->report_ui_array[i+16+16].helper[0] = v;
						grid_report_ui_set_changed_flag(mod, i+16+16);

					}
					if (grid_ui_encoder_array[i].rotation_value < 64){

						grid_ui_encoder_array[i].rotation_value++;
						uint8_t v = 2 * grid_ui_encoder_array[i].rotation_value;

						grid_sys_write_hex_string_value(&mod->report_ui_array[i+16+16].payload[9], 2, v); // LED
						mod->report_ui_array[i+16+16].helper[0] = v;
						grid_report_ui_set_changed_flag(mod, i+16+16);

					}
				
				
					grid_ui_encoder_array[i].last_real_time = grid_sys_rtc_get_time(&grid_sys_state);
				}
				
						
						
			}

				
				
		}

			
	}
		

	grid_module_en16_reva_hardware_transfer_complete = 0;
	grid_module_en16_reva_hardware_start_transfer();
}

void grid_module_en16_reva_hardware_init(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);
	
	
	
	
	
	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
	spi_m_async_set_baudrate(&UI_SPI, 400000);
	
	spi_m_async_get_io_descriptor(&UI_SPI, &grid_module_en16_reva_hardware_io);


	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_module_en16_reva_hardware_transfer_complete_cb);


}

void grid_module_en16_reva_init(struct grid_ui_model* mod){
	
	
	grid_led_init(&grid_led_state, 16);
	
	grid_ui_model_init(mod, 16+16+16);
	
	
	
	// 0 is for mapmode_button
	// 1...16 is for ui_buttons
	for(uint8_t i=0; i<16+16+16; i++){
		
		uint8_t payload_template[30] = {0};
		

		if (i<16){ // ROTATION
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_NOTEON,
			i,
			0,
			GRID_MSG_END_OF_TEXT

			);
			
		}		
		else if (i<16+16){ // BUTTON
		
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
		
				GRID_MSG_START_OF_TEXT,
				GRID_MSG_PROTOCOL_MIDI,
				0, // (cable<<4) + channel
				GRID_MSG_COMMAND_MIDI_NOTEON,
				i,
				0,
				GRID_MSG_END_OF_TEXT

			);
		
		}
		else{ // LED
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_LED,
			0, // layer
			GRID_MSG_COMMAND_LED_SET_PHASE,
			i-16-16,
			0,
			GRID_MSG_END_OF_TEXT

			);
				
		}

		
		uint32_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;

		grid_report_ui_init(mod, i, GRID_REPORT_TYPE_BROADCAST, payload_template, payload_length, helper_template, helper_length);
		
	}
	
	grid_report_sys_init(mod);

	for (uint8_t i = 0; i<16; i++)
	{
		grid_ui_encoder_array[i].controller_number = i;
	}
	
	
	grid_module_en16_reva_hardware_init();
	
	
	grid_module_en16_reva_hardware_start_transfer();
	
}

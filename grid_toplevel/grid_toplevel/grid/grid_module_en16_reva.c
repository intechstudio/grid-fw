#include "grid_module_en16_reva.h"





uint8_t UI_SPI_TX_BUFFER[14] = {0};
uint8_t UI_SPI_RX_BUFFER[14] = {0};
uint8_t UI_SPI_TRANSFER_LENGTH = 10;

volatile uint8_t UI_SPI_DONE = 0;


uint8_t helper[GRID_SYS_BANK_MAXNUMBER][16] = {0};

volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};



uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;










void grid_module_en16_reva_hardware_start_transfer(void){
	

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);

	spi_m_async_enable(&UI_SPI);

	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

void grid_module_en16_reva_hardware_transfer_complete_cb(void){

	/* Transfer completed */

	
	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);


	uint8_t bank = grid_sys_get_bank_num(&grid_sys_state);
	
	if (bank == 255){
		bank=0;
	}


	uint8_t bank_changed = grid_sys_state.bank_changed;
		
	if (bank_changed){
		grid_sys_state.bank_changed = 0;
				
		for (uint8_t i = 0; i<16; i++)
		{
			
// 			uint8_t error = 0;
// 			
// 			uint8_t* message = mod->report_ui_array[i+16+16].payload;
// 			uint8_t actuator = mod->report_ui_array[i+16+16].helper[bank];
// 			
// 			grid_msg_set_parameter(message, GRID_CLASS_LEDPHASE_PHASE_offset  , GRID_CLASS_LEDPHASE_PHASE_length  , actuator, &error);
// 			grid_report_ui_set_changed_flag(mod, i+16+16);


			uint8_t value = helper[bank][i];
			uint8_t res_index = i;
			uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_parameter_list;
			uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
			
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];

			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV7] = value;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV8] = value*2;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14U] = 0;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14L] = 0;
			
			uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_AVC7);

			grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
			
			grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
			
		}
	}


		
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
				

				uint8_t res_index = i;
				uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_parameter_list;						
				uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
					
				if (grid_ui_encoder_array[i].button_value == 0){ // Button Press Event
				
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];
				
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV7] = 127;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV8] = 255;
				
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_DP);

					
				
					grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
					grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
		
					
					
					
				
				}
				else{  // Button Release Event
				
 					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
 					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];
 
 					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV7] = 0;
 					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV8] = 0;

				
 					uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_DR);
					  				
 				
				
					grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
					grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
			
					

				}
			
			

				
				
				
			}
				
			uint8_t a_now = phase_a;
			uint8_t b_now = phase_b;
			
			uint8_t a_prev = grid_ui_encoder_array[i].phase_a_previous;
			uint8_t b_prev = grid_ui_encoder_array[i].phase_b_previous;
			
			int16_t delta = 0;
			
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
				
				volatile uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_ui_encoder_array[i].last_real_time);
				
				if (elapsed_time>400){
					elapsed_time = 400;
				}
				
				if (elapsed_time<20){
					elapsed_time = 20;
				}
			
				
				uint16_t velocityfactor = (160000-elapsed_time*elapsed_time)/60000.0 + 1;
				
				
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
				
				uint8_t command = GRID_PARAMETER_MIDI_CONTROLCHANGE;
				uint8_t controlnumber = i;
				uint8_t value = 0;	
								
				
					
				value = helper[bank][i];
				
				if (value + delta*velocityfactor < 0){
					value = 0;
				}
				else if (value + delta*velocityfactor > 127){
					value = 127;
				}
				else{
					value += delta*velocityfactor;
				}
								
				uint8_t actuator = value*2;
				
				if (value != helper[bank][i]){
						
					helper[bank][i] = value;
					uint8_t res_index = i;
					uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_parameter_list;
					uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};				
															
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];

					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV7] = value;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV8] = value*2;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14U] = 0;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14L] = 0;
					
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_AVC7);

					grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
					
					grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
					
					
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

void grid_module_en16_reva_init(struct grid_report_model* mod){
	
	
	grid_led_lowlevel_init(&grid_led_state, 16);
	grid_report_model_init(mod, 0);
	
	grid_ui_model_init(&grid_ui_state, 16);	
	
	
	for(uint8_t i=0; i<16; i++){
	
		grid_ui_element_init(&grid_ui_state.element[i], GRID_UI_ELEMENT_ENCODER);
		
		if (1){ // ROTATION -> MIDI Control Change
		
		
			uint8_t payload_template[100] = {0};
			sprintf(payload_template, GRID_EVENT_AVC7_ENC GRID_DEFAULT_ACTION_AVC7);
			uint8_t payload_length = strlen(payload_template);

			// Register Absolute Value Change
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_AVC7, payload_template, payload_length);
		
		}
		
		if (1){ // BUTTONS -> MIDI Note On/Off
		
		
			uint8_t payload_template[100] = {0};
		
			sprintf(payload_template, GRID_EVENT_DP_ENC GRID_DEFAULT_ACTION_DP_ENC);
			uint8_t payload_length = strlen(payload_template);
		
			// Register Digital Press Action
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_DP, payload_template, payload_length);
		
			sprintf(payload_template, GRID_EVENT_DR_ENC GRID_DEFAULT_ACTION_DR_ENC);
		
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_DR, payload_template, payload_length);
		
		}	
						
	}
	
	grid_report_sys_init(mod);


	// initialize local encoder helper struct
	for (uint8_t i = 0; i<16; i++)
	{
		grid_ui_encoder_array[i].controller_number = i;
		
		grid_ui_encoder_array[i].button_value = 1;
		grid_ui_encoder_array[i].button_changed = 0; 
		grid_ui_encoder_array[i].rotation_value = 0;
		grid_ui_encoder_array[i].rotation_changed = 1;
		grid_ui_encoder_array[i].rotation_direction = 0;
		grid_ui_encoder_array[i].last_real_time = -1;
		grid_ui_encoder_array[i].velocity = 0;
		grid_ui_encoder_array[i].phase_a_previous = 1;
		grid_ui_encoder_array[i].phase_b_previous = 1;	
		
	}
	
	
	grid_module_en16_reva_hardware_init();
	
	
	grid_module_en16_reva_hardware_start_transfer();
	
}

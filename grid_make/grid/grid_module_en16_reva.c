#include "grid_module_en16_reva.h"





uint8_t UI_SPI_TX_BUFFER[14] = {0};
uint8_t UI_SPI_RX_BUFFER[14] = {0};
uint8_t UI_SPI_TRANSFER_LENGTH = 10;

volatile uint8_t UI_SPI_DONE = 0;


static uint8_t grid_en16_helper_template_e_abs[GRID_SYS_BANK_MAXNUMBER][16] = {0};
static uint8_t grid_en16_helper_template_e_abs_low_velocity[GRID_SYS_BANK_MAXNUMBER][16] = {0};
static uint8_t grid_en16_helper_template_e_abs_high_velocity[GRID_SYS_BANK_MAXNUMBER][16] = {0};
	
static uint8_t grid_en16_helper_template_b_tgl2[GRID_SYS_BANK_MAXNUMBER][16] = {0};
static uint8_t grid_en16_helper_template_b_tgl3[GRID_SYS_BANK_MAXNUMBER][16] = {0};

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


	uint8_t bank_changed = grid_sys_state.bank_active_changed;
		
	if (bank_changed){
		grid_sys_state.bank_active_changed = 0;
				
		for (uint8_t i = 0; i<16; i++)
		{
			

			uint8_t res_index = i;
			int32_t* template_parameter_list = grid_ui_state.bank_list[bank].element_list[res_index].template_parameter_list;
			uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
			
			//BUTTON
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER] = res_index;
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];
		
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL2] = grid_en16_helper_template_b_tgl2[bank][i];
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] = grid_en16_helper_template_b_tgl3[bank][i];
					
			
			//ENCODER
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_NUMBER] = res_index;
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];

			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS] = grid_en16_helper_template_e_abs[bank][i];
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS_VELOCITY_LOW] = grid_en16_helper_template_e_abs_low_velocity[bank][i];
			template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS_VELOCITY_HIGH] = grid_en16_helper_template_e_abs_high_velocity[bank][i];			
					
			grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_INIT);
            
            grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_AVC7);
            				


			
		}
	}


		
	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t j=0; j<16; j++){
		

		uint8_t i = UI_ENCODER_LOOKUP[j];
		

		uint8_t new_value = (UI_SPI_RX_BUFFER[j/2]>>(4*(j%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];
			
		if (old_value != new_value){

            UI_SPI_RX_BUFFER_LAST[j] = new_value;
				
			UI_SPI_DEBUG = j;
            
            // GND Button PhaseB PhaseA
				
			uint8_t button_value = (new_value&0b00000100)?1:0;
            uint8_t phase_a      = (new_value&0b00000010)?1:0;
			uint8_t phase_b      = (new_value&0b00000001)?1:0;
				
			if (button_value != grid_ui_encoder_array[i].button_value){
				// BUTTON CHANGE
				grid_ui_encoder_array[i].button_changed = 1;
				grid_ui_encoder_array[i].button_value = new_value>>2;
				

				uint8_t res_index = i;
				int32_t* template_parameter_list = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[res_index].template_parameter_list;						
				uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
					
				if (grid_ui_encoder_array[i].button_value == 0){ // Button Press Event

					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER] = res_index;
					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];
								
					// Button ABS
					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_ABS] = 127;
					
					// Button TGL2
					if (template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL2] == 0){
						template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL2] = 127;
					}
					else{
						template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL2] = 0;
					}
					
					// Button TGL3
					if (template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] == 0){
						template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] = 63;
					}
					else if (template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] == 63){
						template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] = 127;
					}
					else{
						template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3] = 0;
					}
					
					grid_en16_helper_template_b_tgl2[grid_sys_state.bank_activebank_number][i] = template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL2];
					grid_en16_helper_template_b_tgl3[grid_sys_state.bank_activebank_number][i] = template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_TGL3];
					
					
		
					grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_BC);
					
					
					
				
				}
				else{  // Button Release Event
				
 					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER] = res_index;
 					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index];
 
 					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_CONTROLLER_ABS] = 0;
	
					grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_BC);
					

				}
			
			

				
				
				
			}
				
			uint8_t a_now = phase_a;
			uint8_t b_now = phase_b;
			
			uint8_t a_prev = grid_ui_encoder_array[i].phase_a_previous;
			uint8_t b_prev = grid_ui_encoder_array[i].phase_b_previous;
			
			int16_t delta = 0;
			
//			if (a_now != a_prev){
//				
//				if (b_now != a_now){
//					delta = -1;
//				}
//				else{
//					delta = +1;
//				}
//				
//				
//			}
            
            if (a_now == 1 && b_now == 1){ //detent found
            
                if (b_prev == 0 && grid_ui_encoder_array[i].phase_change_lock == 0){
                    delta = -1;
                    grid_ui_encoder_array[i].phase_change_lock = 1;
                }
                
                if (a_prev == 0 && grid_ui_encoder_array[i].phase_change_lock == 0){
                    delta = 1;
                    grid_ui_encoder_array[i].phase_change_lock = 1;
                }
                
            }
            
            //printf("%d %d %d\n", button_value, phase_a, phase_b);
            
            if (a_now == 0 && b_now == 0){
            
                grid_ui_encoder_array[i].phase_change_lock = 0;
                
            }
			

			
			grid_ui_encoder_array[i].phase_a_previous = a_now;
			grid_ui_encoder_array[i].phase_b_previous = b_now;
						
            
            int16_t delta_low = 0;
            int16_t delta_high = 0;
            
			if (delta != 0){
				
				uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_ui_encoder_array[i].last_real_time);
				
                uint32_t elapsed_ms = elapsed_time/RTC1MS;
                
                
				if (elapsed_ms>25){
					elapsed_ms = 25;
				}
				
				if (elapsed_ms<1){
					elapsed_ms = 1;
				}
			
				
				uint8_t velocityfactor = (25*25-elapsed_ms*elapsed_ms)/150 + 1;
				
				
				grid_ui_encoder_array[i].last_real_time = grid_sys_rtc_get_time(&grid_sys_state);
				
				delta_low =  delta * velocityfactor;			
				delta_high = delta * (velocityfactor * 2 - 1);
                
				

				//CRITICAL_SECTION_ENTER()

                uint8_t res_index = i;
                int32_t* template_parameter_list = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[res_index].template_parameter_list;
                uint8_t grid_module_en16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};

				uint8_t command = GRID_PARAMETER_MIDI_CONTROLCHANGE;
				uint8_t controlnumber = i;
 
                template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_NUMBER] = res_index;
                template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_en16_mux_reversed_lookup[res_index]; 

				uint8_t new_abs_no_velocity_value = grid_en16_helper_template_e_abs[bank][i];
				uint8_t new_abs_low_velocity_value = grid_en16_helper_template_e_abs_low_velocity[bank][i];
				uint8_t new_abs_high_velocity_value = grid_en16_helper_template_e_abs_high_velocity[bank][i];
                
				uint8_t new_rel_no_velocity_value =  template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL];
				uint8_t new_rel_low_velocity_value =  template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL];
				uint8_t new_rel_high_velocity_value =  template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL];
								
				
				if (delta != 0){
                    
                    
                    // ABSOLUTE NO VELOCITY						
					if (new_abs_no_velocity_value + delta < 0){
						new_abs_no_velocity_value = 0;
					}
					else if (new_abs_no_velocity_value + delta > 127){
						new_abs_no_velocity_value = 127;
					}
					else{
						new_abs_no_velocity_value += delta;
					}	
                    
                    // ABSOLUTE LOW VELOCITY						
					if (new_abs_low_velocity_value + delta_low < 0){
						new_abs_low_velocity_value = 0;
					}
					else if (new_abs_low_velocity_value + delta_low > 127){
						new_abs_low_velocity_value = 127;
					}
					else{
						new_abs_low_velocity_value += delta_low;
					}	
  					
                    // ABSOLUTE HIGH VELOCITY						
					if (new_abs_high_velocity_value + delta_high < 0){
						new_abs_high_velocity_value = 0;
					}
					else if (new_abs_high_velocity_value + delta_high > 127){
						new_abs_high_velocity_value = 127;
					}
					else{
						new_abs_high_velocity_value += delta_high;
					}	



                    // RELATIVE NO VELOCITY
					if (new_rel_no_velocity_value == 255){
						if (delta>0){
							new_rel_no_velocity_value = 65;
						}
						else{
							new_rel_no_velocity_value = 63;
						}
					}
					else{
						new_rel_no_velocity_value += delta;
					}
                    
                    // RELATIVE LOW VELOCITY
					if (new_rel_low_velocity_value == 255){
                        
                        new_rel_low_velocity_value = 64 + delta_low;
					
					}
					else{
						new_rel_low_velocity_value += delta_low;
					}
                    
                    // RELATIVE HIGH VELOCITY
					if (new_rel_high_velocity_value == 255){
				
                        new_rel_high_velocity_value = 64 + delta_high;
						
					}
					else{
						new_rel_high_velocity_value += delta_high;
					}
	
                    
                    
					
					template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL] = new_rel_no_velocity_value;
                    template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL_VELOCITY_LOW] = new_rel_low_velocity_value;
                    template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_REL_VELOCITY_HIGH] = new_rel_high_velocity_value;
				
                    
                    
                    
                    
                    
                    
                    
                    if (button_value == 1){
                        
                        // ABS is only updated if nonpush rotation event happened
                        grid_en16_helper_template_e_abs[bank][i] = new_abs_no_velocity_value;
                        grid_en16_helper_template_e_abs_low_velocity[bank][i] = new_abs_low_velocity_value;
                        grid_en16_helper_template_e_abs_high_velocity[bank][i] = new_abs_high_velocity_value;
                        
                        // ABS no velocity
                        template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS] = new_abs_no_velocity_value;
                        
                        // ABS low velocity
                        template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS_VELOCITY_LOW] = new_abs_low_velocity_value;
                        
                        // ABS high velocity
                        template_parameter_list[GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH + GRID_TEMPLATE_E_PARAMETER_CONTROLLER_ABS_VELOCITY_HIGH] = new_abs_high_velocity_value;
                        
                        
                        grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_AVC7);				
                    }
                    else{
                        grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_ENCPUSHROT);

                    }
					
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
	spi_m_async_set_baudrate(&UI_SPI, 1000000); // was 400000 check clock div setting
	
	spi_m_async_get_io_descriptor(&UI_SPI, &grid_module_en16_reva_hardware_io);


	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_module_en16_reva_hardware_transfer_complete_cb);


}

void grid_module_en16_reva_init(){
	
	
	grid_led_lowlevel_init(&grid_led_state, 16);
	
	grid_ui_model_init(&grid_ui_state, GRID_SYS_BANK_MAXNUMBER);	


	
	for (uint8_t i=0; i<GRID_SYS_BANK_MAXNUMBER; i++)
	{
		
		grid_ui_bank_init(&grid_ui_state, i, 16);	
		
		for(uint8_t j=0; j<16; j++){
		
			grid_ui_element_init(&grid_ui_state.bank_list[i], j, GRID_UI_ELEMENT_ENCODER);	
		
		}		
		
		
	}
	


	// initialize local encoder helper struct
	for (uint8_t j = 0; j<16; j++)
	{
		grid_ui_encoder_array[j].controller_number = j;
		
		grid_ui_encoder_array[j].button_value = 1;
		grid_ui_encoder_array[j].button_changed = 0; 
		grid_ui_encoder_array[j].rotation_value = 0;
		grid_ui_encoder_array[j].rotation_changed = 1;
		grid_ui_encoder_array[j].rotation_direction = 0;
		grid_ui_encoder_array[j].last_real_time = -1;
		grid_ui_encoder_array[j].velocity = 0;
		grid_ui_encoder_array[j].phase_a_previous = 1;
		grid_ui_encoder_array[j].phase_b_previous = 1;	
        
        grid_ui_encoder_array[j].phase_change_lock = 0;
		
	}
	
	
	grid_module_en16_reva_hardware_init();
	
	
	grid_module_en16_reva_hardware_start_transfer();
	
}

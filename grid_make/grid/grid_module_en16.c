#include "grid_module_en16.h"





uint8_t UI_SPI_TX_BUFFER[14] = {0};
uint8_t UI_SPI_RX_BUFFER[14] = {0};
uint8_t UI_SPI_TRANSFER_LENGTH = 10;

volatile uint8_t UI_SPI_DONE = 0;


volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};

uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;

uint8_t UI_ENCODER_LOOKUP_REVERSED[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
			


void grid_module_en16_hardware_start_transfer(void){
	

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);

	spi_m_async_enable(&UI_SPI);

	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

void grid_module_en16_hardware_transfer_complete_cb(void){

	/* Transfer completed */

	
	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

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

			// encoder phase decode magic

			uint8_t a_now = phase_a;
			uint8_t b_now = phase_b;
			
			uint8_t a_prev = grid_ui_encoder_array[i].phase_a_previous;
			uint8_t b_prev = grid_ui_encoder_array[i].phase_b_previous;
			
			int16_t delta = 0;
			
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
            
            if (a_now == 0 && b_now == 0){
            
                grid_ui_encoder_array[i].phase_change_lock = 0;
    
            }
			
			grid_ui_encoder_array[i].phase_a_previous = a_now;
			grid_ui_encoder_array[i].phase_b_previous = b_now;
						

			// Evaluate the results

			if (button_value != grid_ui_encoder_array[i].button_value){  // The button has changed
				// BUTTON CHANGE
				grid_ui_encoder_array[i].button_changed = 1;
				grid_ui_encoder_array[i].button_value = new_value>>2;


				uint8_t res_index = i;
				int32_t* template_parameter_list = grid_ui_state.element_list[res_index].template_parameter_list;					


				if (grid_ui_encoder_array[i].button_value == 0){ // Button Press
		
					// Button ABS
					if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0){

						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = 127;
					}
					else{
						// IMPLEMENT STEP TOGGLE HERE
						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = 127;
					}
						
					struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
					grid_ui_event_trigger(eve);	
		
				}
				else{  // Button Release
				
					// Button ABS
					if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0){

						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = 0;
					}
					else{
						// IMPLEMENT STEP TOGGLE HERE
						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = 0;
					}
								
					struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
					grid_ui_event_trigger(eve);	
					
				}
			
			}
				
			if (delta != 0){ // The encoder rotation has changed
				

				uint8_t res_index = i;
                int32_t* template_parameter_list = grid_ui_state.element_list[res_index].template_parameter_list;
   			
				uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_ui_encoder_array[i].last_real_time);
				grid_ui_encoder_array[i].last_real_time = grid_sys_rtc_get_time(&grid_sys_state);

                uint32_t elapsed_ms = elapsed_time/RTC1MS;

				template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] = elapsed_ms;
                  
				if (elapsed_ms>25){
					elapsed_ms = 25;
				}
				
				if (elapsed_ms<1){
					elapsed_ms = 1;
				}
								
					
				// implement configurable velocity parameters here	
				uint8_t velocityfactor = (25*25-elapsed_ms*elapsed_ms)/150 + 1;		
				int32_t delta_velocity = delta * (velocityfactor * 2 - 1);


				int32_t new_value = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];
				int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
				int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];


				if (new_value + delta_velocity < min){
					new_value = min;
				}
				else if (new_value + delta_velocity > max){
					new_value = max;
				}
				else{
					new_value += delta_velocity;
				}	
				
				template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_value;
		
				struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_EC);
				grid_ui_event_trigger(eve);	
			
				
			}
			
		}
			
	}
		

	grid_module_en16_hardware_transfer_complete = 0;
	grid_module_en16_hardware_start_transfer();
}


void grid_module_en16_hardware_init(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);


	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
	spi_m_async_set_baudrate(&UI_SPI, 1000000); // was 400000 check clock div setting
	
	spi_m_async_get_io_descriptor(&UI_SPI, &grid_module_en16_hardware_io);

	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_module_en16_hardware_transfer_complete_cb);

}

void grid_module_en16_init(){
	
	
	grid_led_lowlevel_init(&grid_led_state, 16);
	
	grid_ui_model_init(&grid_ui_state, 16);	
		
	for(uint8_t j=0; j<16; j++){
	
		grid_ui_element_init(&grid_ui_state, j, GRID_UI_ELEMENT_ENCODER);

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
	
	grid_module_en16_hardware_init();
	
	
	grid_module_en16_hardware_start_transfer();
	
}
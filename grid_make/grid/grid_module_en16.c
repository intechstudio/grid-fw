#include "grid_module_en16.h"




static struct grid_ui_encoder grid_ui_encoder_array[16] = {0};




static uint8_t UI_SPI_DEBUG;







struct io_descriptor *grid_module_en16_hardware_io;


static volatile uint8_t grid_module_en16_hardware_transfer_complete;

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};
static uint8_t UI_SPI_TRANSFER_LENGTH = 10;

static volatile uint8_t UI_SPI_DONE = 0;
static volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};

static uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;

static uint8_t UI_ENCODER_LOOKUP_REVERSED[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
		
static uint32_t encoder_last_real_time[16] = {0};
static uint32_t button_last_real_time[16] = {0};


static void grid_module_en16_hardware_start_transfer(void){
	

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);

	spi_m_async_enable(&UI_SPI);

	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

static void grid_module_en16_hardware_transfer_complete_cb(void){

	/* Transfer completed */

	//printf("%d\r\n", _irq_get_current());
	
	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t j=0; j<16; j++){

		uint8_t new_value = (UI_SPI_RX_BUFFER[j/2]>>(4*(j%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];

		uint8_t i = UI_ENCODER_LOOKUP[j];

		// limit lastrealtime
		uint32_t button_elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, button_last_real_time[i]);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, button_last_real_time[i])){
			button_last_real_time[i] = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			button_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}
		uint32_t encoder_elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, encoder_last_real_time[i]);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, encoder_last_real_time[i])){
			encoder_last_real_time[i] = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			encoder_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}
				
			
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

				// update lastrealtime
				button_last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state); 
				template_parameter_list[GRID_LUA_FNC_E_BUTTON_ELAPSED_index] = button_elapsed_time/RTC1MS;




				if (grid_ui_encoder_array[i].button_value == 0){ // Button Press
		
					template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] = 127;

					// Button ABS
					if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0){

						int32_t max = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index];
						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = max;
					}
					else{
						// IMPLEMENT STEP TOGGLE HERE					// Toggle

						int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
						int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
						int32_t steps = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index];
						int32_t last = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];

						int32_t next = last + (max - min)/steps;

						if (next > max){

							//overflow
							next = min;
						}

						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = next;
					}
						
					struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
					
					if (grid_ui_state.ui_interaction_enabled){
						grid_ui_event_trigger(eve);	
					}	
		
				}
				else{  // Button Release
				
					template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] = 0;

					// Button ABS
					if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0){

						int32_t min = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index];

						template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = min;
					}
					else{
						// IMPLEMENT STEP TOGGLE HERE

					}
								
					struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
				
					if (grid_ui_state.ui_interaction_enabled){
						grid_ui_event_trigger(eve);	
					}	
					
				}
			
			}
				
			if (delta != 0){ // The encoder rotation has changed


				uint8_t res_index = i;
                int32_t* template_parameter_list = grid_ui_state.element_list[res_index].template_parameter_list;
			
				// update lastrealtime
				encoder_last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state); 
				template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] = encoder_elapsed_time/RTC1MS;


				int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
				int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];

				double elapsed_ms = encoder_elapsed_time/RTC1MS;

				if (elapsed_ms>25){
					elapsed_ms = 25;
				}
				
				if (elapsed_ms<1){
					elapsed_ms = 1;
				}

				double minmaxscale = (max-min)/128.0;	
				
				double velocityparam = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VELOCITY_index]/100.0;
						
					
				// implement configurable velocity parameters here	
				double velocityfactor = ((25*25-elapsed_ms*elapsed_ms)/75.0)*minmaxscale*velocityparam + 1.0;		
				int32_t delta_velocity = delta * velocityfactor;

				int32_t old_value = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];

				template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] += delta_velocity;

				if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 0){ // Absolute

					int32_t new_value = 0;

					if (old_value + delta_velocity < min){
						new_value = min;
					}
					else if (old_value + delta_velocity > max){
						new_value = max;
					}
					else{
						new_value = old_value + delta_velocity;
					}	
					
					template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_value;

				}
				else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 1){ // Relative

					int32_t new_value = 0;

					if (old_value + delta_velocity < min){
						new_value = min;
					}
					else if (old_value + delta_velocity > max){
						new_value = max;
					}
					else{
						new_value = old_value + delta_velocity;
					}	
					
					template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_value;
					
				}
				else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 2){ // Relative 2's complement

					// Two's complement magic 7 bit signed variable
				
				   	int32_t old_twoscomplement = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];
					
					uint8_t old_8bit_extended_twoscomplement;

					//Limit to signed -64 +63 range
					if (old_twoscomplement>127){
						old_8bit_extended_twoscomplement = 127;
					}
					if (old_twoscomplement<0){
						old_8bit_extended_twoscomplement = 0;
					}
					
					if (old_twoscomplement > 63){ // extend sign bit to 8 bit size
						old_8bit_extended_twoscomplement+=128;
					}
					
					short unsigned val = old_8bit_extended_twoscomplement;

					int8_t old_signed;

					if (old_8bit_extended_twoscomplement>127){ // negative number
						old_signed = -( (~old_8bit_extended_twoscomplement) + 1 + 256);	
					}
					else{ // positive number
						old_signed = -(~old_8bit_extended_twoscomplement) - 1;
					}

					int16_t new_signed = old_signed - delta_velocity;
				
					//Limit to signed -64 +63 range
					if (new_signed<-64){
						new_signed = -64;
					}

					if (new_signed>63){
						new_signed = 63;
					}

					int8_t new_signed_8bit = new_signed;

					// Two's complement magic
					uint8_t new_twoscomplement = (~new_signed_8bit)+1;
							
					// reduce the result to 7 bit length
					uint8_t new_7bit_twoscomplement = new_twoscomplement & 127;

					template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_7bit_twoscomplement;
					
				}


		
				struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_EC);
				
				if (grid_ui_state.ui_interaction_enabled){
					grid_ui_event_trigger(eve);	
				}	
			
				
			}
			
		}
			
	}
		

	grid_module_en16_hardware_transfer_complete = 0;
	grid_module_en16_hardware_start_transfer();
}


static void grid_module_en16_hardware_init(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);


	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
	spi_m_async_set_baudrate(&UI_SPI, 1000000); // was 400000 check clock div setting
	
	spi_m_async_get_io_descriptor(&UI_SPI, &grid_module_en16_hardware_io);

	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_module_en16_hardware_transfer_complete_cb);

}

void grid_module_en16_init(){
	
	
	grid_led_init(&grid_led_state, 16);
	
	grid_ui_model_init(&grid_ui_state, &GRID_PORT_U, 16+1); // +1 for the system element	
		
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
		grid_ui_encoder_array[j].velocity = 0;
		grid_ui_encoder_array[j].phase_a_previous = 1;
		grid_ui_encoder_array[j].phase_b_previous = 1;	
        
        grid_ui_encoder_array[j].phase_change_lock = 0;
		
	}
	
	grid_module_en16_hardware_init();
	
	
	grid_module_en16_hardware_start_transfer();
	
}

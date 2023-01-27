#include "grid_module_bu16.h"

volatile uint8_t grid_module_bu16_hardware_transfer_complete = 0;
volatile uint8_t grid_module_bu16_mux = 0;


volatile uint8_t grid_module_bu16_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
uint8_t grid_module_bu16_mux_reversed_lookup[16]      = {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};	

static uint32_t last_real_time[16] = {0};
	
void grid_module_bu16_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);

}

static void grid_module_bu16_hardware_transfer_complete_cb(void){
		
	if (grid_module_bu16_hardware_transfer_complete == 0){
		grid_module_bu16_hardware_transfer_complete++;
		return;
	}

	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_bu16_mux_lookup[grid_module_bu16_mux+8];
	uint8_t adc_index_1 = grid_module_bu16_mux_lookup[grid_module_bu16_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_bu16_mux++;
	grid_module_bu16_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_bu16_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_bu16_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_bu16_mux/4%2);
	
	
	adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
	adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
	
	uint8_t result_index[2] = {0};
	uint8_t result_value[2] = {0};
	uint8_t result_valid[2] = {0};
		
	result_index[0] = adc_index_0;
	result_index[1] = adc_index_1;


	uint8_t adcresult_0_valid = 0;
	
	if (adcresult_0>60000){
		result_value[0] = 0;
		result_valid[0] = 1;
	}
	else if (adcresult_0<200){
		result_value[0] = 127;
		result_valid[0] = 1;
	}
		
	uint8_t adcresult_1_valid = 0;
	
	if (adcresult_1>60000){
		result_value[1] = 0;
		result_valid[1] = 1;
	}
	else if (adcresult_1<200){
		result_value[1] = 127;
		result_valid[1] = 1;
	}



	// Process both ADC results
	
	for (uint8_t i=0; i<2; i++)
	{

		// Helper variable for readability
		uint8_t res_index = result_index[i];
		uint8_t res_valid = result_valid[i];
		uint8_t res_value = result_value[i];

		// limit lastrealtime
		uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, last_real_time[res_index]);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, last_real_time[res_index])){
			last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}

		
		int32_t* template_parameter_list = grid_ui_state.element_list[res_index].template_parameter_list;		
		
		if (res_value != template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] && res_valid == 1){
			// button change happened
			template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = res_value;
			
			// update lastrealtime
			last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state); 
			template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = elapsed_time/RTC1MS;

			if (res_value != 0){ // Button Press Event
					
				if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0){
					
					// Button ABS
					int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
					template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = max;
		
				}
				else{

					// Toggle

					int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
					int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
					int32_t steps = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index];
					int32_t last = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];

					int32_t next = last + (max - min)/steps;

					if (next > max){

						//overflow
						next = min;
					}

					template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = next;

				}
                
				struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[res_index], GRID_UI_EVENT_BC);
				
				if (grid_ui_state.ui_interaction_enabled){
					grid_ui_event_trigger(eve);	
				}	
				
			}
			else{  // Button Release Event

				if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0){
					
					// Button ABS						
					int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
					template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = min;
				
				}
 				else{

					// Toggle

				}               
				
				struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[res_index], GRID_UI_EVENT_BC);
				
				if (grid_ui_state.ui_interaction_enabled){
					grid_ui_event_trigger(eve);	
				}	

			}
			
		}

	}
	
	grid_module_bu16_hardware_transfer_complete = 0;
	grid_module_bu16_hardware_start_transfer();
}


void grid_module_bu16_hardware_init(void){

	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}

void grid_module_bu16_init(){

	grid_led_init(&grid_led_state, 16);
	
	grid_ui_model_init(&grid_ui_state, &GRID_PORT_U, 16+1); // +1 for the system element

	for (uint8_t j=0; j<16; j++){

		grid_ui_element_init(&grid_ui_state, j, GRID_UI_ELEMENT_BUTTON);

	}					
	

	grid_ui_element_init(&grid_ui_state, grid_ui_state.element_list_length-1, GRID_UI_ELEMENT_SYSTEM);

	grid_module_bu16_hardware_init();
	grid_module_bu16_hardware_start_transfer();

};

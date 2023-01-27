#include "grid_module.h"

void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui){
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(ain, 16, 5);
	grid_led_init(led, 12);	
	
	grid_ui_model_init(ui, &GRID_PORT_U, 12+1); // +1 for the system element

	for(uint8_t j=0; j<8; j++){
			
		grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
	
	}	

	for(uint8_t j=8; j<12; j++){
			
		grid_ui_element_init(ui, j, GRID_UI_ELEMENT_BUTTON);
	
	}		


	grid_ui_element_init(ui, ui->element_list_length-1, GRID_UI_ELEMENT_SYSTEM);
	
}

void grid_module_pbf4_store_input(uint8_t input_channel, uint32_t* last_real_time, uint16_t value, uint8_t adc_bit_depth){

	const uint16_t adc_max_value = (1<<adc_bit_depth) - 1;

	int32_t* template_parameter_list = grid_ui_state.element_list[input_channel].template_parameter_list;	


	if (input_channel > 7){ // BUTTON

		uint8_t result_valid = 0;

	
		if (value>adc_max_value*0.9){
			value = 0;
			result_valid = 1;
		}
		else if (value<adc_max_value*0.01){
			value = 127;
			result_valid = 1;
		}

		// schmitt trigger test faild, result not valid
		if (result_valid == 0){
			return;
		}
	

		// limit lastrealtime
		uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, *last_real_time);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, *last_real_time)){
			*last_real_time = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}


		// value is the same as it was last time 
		if (value == template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index]){
			return;
		}
		
	
		// button change happened
		template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = value;
		
		// update lastrealtime
		*last_real_time = grid_sys_rtc_get_time(&grid_sys_state); 
		template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = elapsed_time/RTC1MS;


		if (value != 0){ // Button Press Event

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
			
			struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_BC);
			
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
					
			struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_BC);
			
			if (grid_ui_state.ui_interaction_enabled){
				grid_ui_event_trigger(eve);	
			}

		}
		


		
		

	}
	else{ // POTENTIOMETER OR FADER
		

		int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

		grid_ain_add_sample(&grid_ain_state, input_channel, value, adc_bit_depth, (uint8_t) resolution);

		// limit lastrealtime
		uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, *last_real_time);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, *last_real_time)){
			*last_real_time = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}

		if (grid_ain_get_changed(&grid_ain_state, input_channel)){

			// update lastrealtime
			*last_real_time = grid_sys_rtc_get_time(&grid_sys_state); 
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] = elapsed_time/RTC1MS;

			int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

			if (resolution < 1){
				resolution = 1;
			}
			else if (resolution > 12){
				resolution = 12;
			}

			int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
			int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

			int32_t next = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, min, max);
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;

			// for display in editor
			int32_t state = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, 0, 127);
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = state;

	
			struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_AC);
			
			if (grid_ui_state.ui_interaction_enabled){
				grid_ui_event_trigger(eve);	
			}	
			
		}

	}

}


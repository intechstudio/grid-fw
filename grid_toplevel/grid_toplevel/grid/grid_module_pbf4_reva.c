#include "grid_module_pbf4_reva.h"

volatile uint8_t grid_module_pbf4_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_pbf4_revb_mux =0;
volatile uint8_t grid_module_pbf4_reva_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

static uint8_t helper[16] = {0};

void grid_module_pbf4_reva_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);
	
}

void grid_module_pbf4_reva_hardware_transfer_complete_cb(void){
	
	if (grid_module_pbf4_reva_hardware_transfer_complete == 0){
		grid_module_pbf4_reva_hardware_transfer_complete++;
		return;
	}
	
	uint8_t bank_changed = grid_sys_state.bank_active_changed;
	
	if (bank_changed){
		grid_sys_state.bank_active_changed = 0;

		
		for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){
			
			// action template bug fix try
			grid_ui_state.element[i].template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = i;
			
			uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[i], GRID_UI_EVENT_INIT);
			
			grid_ui_event_template_action(&grid_ui_state.element[i], event_index);
			grid_ui_event_trigger(&grid_ui_state.element[i].event_list[event_index]);
			

		}
		
	}	
	
	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_pbf4_reva_mux_lookup[grid_module_pbf4_reva_mux+8];
	uint8_t adc_index_1 = grid_module_pbf4_reva_mux_lookup[grid_module_pbf4_reva_mux+0];
	

	
	/* Update the multiplexer */
	
	grid_module_pbf4_reva_mux++;
	grid_module_pbf4_reva_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_pbf4_reva_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_pbf4_reva_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_pbf4_reva_mux/4%2);
	
	
	
	adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
	adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
	

	// FAKE CALIBRATION
	uint32_t input_0 = adcresult_0*1.03;
	if (input_0 > (1<<16)-1){
		input_0 = (1<<16)-1;
	}
	adcresult_0 = input_0;
	
	uint32_t input_1 = adcresult_1*1.03;
	if (input_1 > (1<<16)-1){
		input_1 = (1<<16)-1;
	}
	adcresult_1 = input_1;


	if (adc_index_1 == 8 || adc_index_1 == 9){
		
	}
	else if (adc_index_0 > 13){ // BUTTON

		uint8_t result_index[2] = {0};
		uint8_t result_value[2] = {0};
		uint8_t result_valid[2] = {0};
		
		result_index[0] = adc_index_0-4;
		result_index[1] = adc_index_1-4;
		
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


		uint8_t grid_module_pbf4_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};

		// Process both ADC results
	
		for (uint8_t i=0; i<2; i++)
		{
		
			// Helper variable for readability
			uint8_t res_index = result_index[i];
			uint8_t res_valid = result_valid[i];
			uint8_t res_value = result_value[i];
		
			uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_parameter_list;
		
			if (res_value != helper[res_index] && res_valid == 1){
			
			
				if (helper[res_index] == 0){ // Button Press Event
				
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_pbf4_mux_reversed_lookup[res_index];

					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV7] = 127;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV8] = 255;

				
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_DP);

					grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
				
					grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
				
					helper[result_index[i]] = res_value;
				
				}
				else{  // Button Release Event
				
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_pbf4_mux_reversed_lookup[res_index];

					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV7] = 0;
					template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_DV8] = 0;

				
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_DR);
				
					grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
				
					grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);

					helper[result_index[i]] = res_value;
				}
			
			}

		}
		

	}
	else{ // POTENTIOMETER OR FADER
		
		if (adc_index_1 == 0 || adc_index_1 == 1){
			// invert pot polarity
			grid_ain_add_sample(adc_index_0, (1<<16)-1-adcresult_0);
			grid_ain_add_sample(adc_index_1, (1<<16)-1-adcresult_1);
			
		}
		else{
			// normal fader polarity			
			grid_ain_add_sample(adc_index_0, adcresult_0);
			grid_ain_add_sample(adc_index_1, adcresult_1);
			
		}
			

		uint8_t result_index[2] = {0};
	
		result_index[0] = adc_index_0;
		result_index[1] = adc_index_1;


		uint8_t grid_module_pbf4_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};

		// Process both ADC results

		for (uint8_t i=0; i<2; i++)
		{
		
			// Helper variable for readability
			uint8_t res_index = result_index[i];

			uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_parameter_list;
		
			if (grid_ain_get_changed(res_index)){
			
				uint8_t res_value = grid_ain_get_average(res_index, 7);
			
			
				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_pbf4_mux_reversed_lookup[res_index];

				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV7] = grid_ain_get_average(res_index, 7);
				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV8] = grid_ain_get_average(res_index, 8);
				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14U] = 0;
				template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14L] = 0;
			
				uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_AVC7);

				grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
			
				grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);
			
			}

		}
	

	}

	

	
	
	
	
	
	grid_module_pbf4_reva_hardware_transfer_complete = 0;
	grid_module_pbf4_reva_hardware_start_transfer();
}

void grid_module_pbf4_reva_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_pbf4_reva_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_pbf4_reva_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}




void grid_module_pbf4_reva_init(){
	
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5, 14, 7);
	grid_led_lowlevel_init(&grid_led_state, 12);	
	
	grid_ui_model_init(&grid_ui_state, 12);
	
	
	
	for(uint8_t i=0; i<12; i++){
		
		uint8_t payload_template[GRID_UI_ACTION_STRING_LENGTH] = {0};
		
		if (i<8){ // PORENTIOMETERS & FADERS -> MIDI Control Change
			
			grid_ui_element_init(&grid_ui_state.element[i], GRID_UI_ELEMENT_POTENTIOMETER);
	
			uint8_t payload_template[100] = {0};
			sprintf(payload_template, GRID_EVENT_AVC7_POT GRID_DEFAULT_ACTION_AVC7);
			uint8_t payload_length = strlen(payload_template);

			// Register Absolute Value Change
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_AVC7, payload_template, payload_length);		
			
		}
		else{ // BUTTONS -> MIDI Note On/Off
			
			grid_ui_element_init(&grid_ui_state.element[i], GRID_UI_ELEMENT_BUTTON);
						
		
			uint8_t payload_template[100] = {0};
			
			sprintf(payload_template, GRID_EVENT_DP_BUT GRID_DEFAULT_ACTION_DP);
			uint8_t payload_length = strlen(payload_template);
			
			// Register Digital Press Action
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_DP, payload_template, payload_length);
			
			sprintf(payload_template, GRID_EVENT_DR_BUT GRID_DEFAULT_ACTION_DR);
			
			grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_DR, payload_template, payload_length);		
									
		}
		
		uint8_t init_action[GRID_UI_ACTION_STRING_LENGTH] = {0};
		sprintf(init_action, GRID_DEFAULT_ACTION_INIT);
		uint8_t init_length = strlen(init_action);
				
		grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_INIT, init_action, init_length);
		
	}
			
	grid_module_pbf4_reva_hardware_init();
	grid_module_pbf4_reva_hardware_start_transfer();
	
}
#include "grid_module_bu16_revb.h"

volatile uint8_t grid_module_bu16_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_bu16_revb_mux = 0;


volatile uint8_t grid_module_bu16_revb_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
uint8_t grid_module_bu16_mux_reversed_lookup[16]      = {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};	

	
void grid_module_bu16_revb_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);

}

static void grid_module_bu16_revb_hardware_transfer_complete_cb(void){
		
	if (grid_module_bu16_revb_hardware_transfer_complete == 0){
		grid_module_bu16_revb_hardware_transfer_complete++;
		return;
	}
	
	uint8_t bank = grid_sys_get_bank_num(&grid_sys_state);
	
	if (bank == 255){
		bank=0;
	}
	
	uint8_t bank_changed = grid_sys_state.bank_active_changed;
	
	if (bank_changed){
		grid_sys_state.bank_active_changed = 0;

		for (uint8_t i=0; i<grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list_length; i++){
			
			grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_INIT);
			grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_BC);
		}
		
	}
	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_bu16_revb_mux_lookup[grid_module_bu16_revb_mux+8];
	uint8_t adc_index_1 = grid_module_bu16_revb_mux_lookup[grid_module_bu16_revb_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_bu16_revb_mux++;
	grid_module_bu16_revb_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_bu16_revb_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_bu16_revb_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_bu16_revb_mux/4%2);
	
	
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
		result_value[0] = 1;
		result_valid[0] = 1;
	}
		
	uint8_t adcresult_1_valid = 0;
	
	if (adcresult_1>60000){
		result_value[1] = 0;
		result_valid[1] = 1;
	}
	else if (adcresult_1<200){
		result_value[1] = 1;
		result_valid[1] = 1;
	}



	// Process both ADC results
	
	for (uint8_t i=0; i<2; i++)
	{
		
		// Helper variable for readability
		uint8_t res_index = result_index[i];
		uint8_t res_valid = result_valid[i];
		uint8_t res_value = result_value[i];
		
		int32_t* template_parameter_list = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[res_index].template_parameter_list;		
		
		if (res_value != template_parameter_list[GRID_TEMPLATE_B_BUTTON_STATE] && res_valid == 1){
			// button change happened
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_STATE] = res_value;
			
			if (res_value == 0){ // Button Press Event


				if (template_parameter_list[GRID_TEMPLATE_B_BUTTON_MODE] == 0){
					
					// Button ABS
					template_parameter_list[GRID_TEMPLATE_B_BUTTON_VALUE] = template_parameter_list[GRID_TEMPLATE_B_BUTTON_MAX];
				
				}
				else{

					// Toggle

					int32_t min = template_parameter_list[GRID_TEMPLATE_B_BUTTON_MIN];
					int32_t max = template_parameter_list[GRID_TEMPLATE_B_BUTTON_MAX];
					int32_t steps = template_parameter_list[GRID_TEMPLATE_B_BUTTON_MODE];
					int32_t last = template_parameter_list[GRID_TEMPLATE_B_BUTTON_VALUE];

					int32_t next = last + (max - min)/steps;

					if (next > max){

						//overflow
						next = min;
					}

					template_parameter_list[GRID_TEMPLATE_B_BUTTON_VALUE] = next;

				}
                
				grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, res_index, GRID_UI_EVENT_BC);
				
			}
			else{  // Button Release Event
				
				if (template_parameter_list[GRID_TEMPLATE_B_BUTTON_MODE] == 0){
					
					// Button ABS
					template_parameter_list[GRID_TEMPLATE_B_BUTTON_VALUE] = template_parameter_list[GRID_TEMPLATE_B_BUTTON_MIN];
				
				}
 				else{

					// Toggle

				}               
				
				grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, res_index, GRID_UI_EVENT_BC);

			}
			
		}

	}
	
	grid_module_bu16_revb_hardware_transfer_complete = 0;
	grid_module_bu16_revb_hardware_start_transfer();
}

void grid_module_bu16_revb_hardware_init(void){

	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}

void grid_module_bu16_revb_init(){

	grid_led_lowlevel_init(&grid_led_state, 16);
	
	grid_ui_model_init(&grid_ui_state, GRID_SYS_BANK_MAXNUMBER);
	
	for (uint8_t i=0; i<GRID_SYS_BANK_MAXNUMBER; i++){
		
		grid_ui_bank_init(&grid_ui_state, i, 16);
		
		for (uint8_t j=0; j<16; j++){

			grid_ui_element_init(&grid_ui_state.bank_list[i], j, GRID_UI_ELEMENT_BUTTON);

			int32_t* template_parameter_list = grid_ui_state.bank_list[i].element_list[j].template_parameter_list;

			template_parameter_list[GRID_TEMPLATE_B_BUTTON_ID] 		= j;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_NUMBER] 	= j;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_VALUE] 	= 0;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_MIN] 	= 0;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_MAX] 	= 127;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_MODE] 	= 0;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_ELAPSED] = 0;
			template_parameter_list[GRID_TEMPLATE_B_BUTTON_STATE] 	= 0;

		}					
		
	}
				
	grid_module_bu16_revb_hardware_init();
	grid_module_bu16_revb_hardware_start_transfer();

};

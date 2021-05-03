#include "grid_module_po16_revb.h"

volatile uint8_t grid_module_po16_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_po16_revb_mux =0;
volatile uint8_t grid_module_po16_revb_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
volatile uint8_t grid_module_po16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};

void grid_module_po16_revb_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);
	
}

static void grid_module_po16_revb_hardware_transfer_complete_cb(void){

	if (grid_module_po16_revb_hardware_transfer_complete == 0){
		grid_module_po16_revb_hardware_transfer_complete++;
		return;
	}
	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_po16_revb_mux_lookup[grid_module_po16_revb_mux+8];
	uint8_t adc_index_1 = grid_module_po16_revb_mux_lookup[grid_module_po16_revb_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_po16_revb_mux++;
	grid_module_po16_revb_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_po16_revb_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_po16_revb_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_po16_revb_mux/4%2);
	
	adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
	adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);

	// FAKE CALIBRATION to compensate oversampling and decimation
	uint32_t input_0 = adcresult_0*1.03;	 // 1.03
	if (input_0 > (1<<16)-1){
		input_0 = (1<<16)-1;
	}
	adcresult_0 = input_0;
	
	uint32_t input_1 = adcresult_1*1.03;	
	if (input_1 > (1<<16)-1){
		input_1 = (1<<16)-1;
	}
	adcresult_1 = input_1;

	// POT POLARITY IS REVERSED ON PO16_RevC
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevC){
		
		// Reverse the 16bit result
		adcresult_0	= 65535 - adcresult_0;
		adcresult_1 = 65535 - adcresult_1;
	}
		
    uint8_t resolution_0 = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[adc_index_0].template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];
    uint8_t resolution_1 = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[adc_index_1].template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

	grid_ain_add_sample(adc_index_0, adcresult_0, resolution_0);
	grid_ain_add_sample(adc_index_1, adcresult_1, resolution_1);


	uint8_t result_index[2] = {0};
	
	result_index[0] = adc_index_0;
	result_index[1] = adc_index_1;



	// Process both ADC results

	for (uint8_t i=0; i<2; i++)
	{
	
		// Helper variable for readability
		uint8_t res_index = result_index[i];

		int32_t* template_parameter_list = grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list[res_index].template_parameter_list;

		if (grid_ain_get_changed(res_index)){
                

			int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

			if (resolution < 1){
				resolution = 1;
			}
			else if (resolution > 12){
				resolution = 12;
			}

			int32_t value = grid_ain_get_average(res_index);

			int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
			int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

			// map the input range to the output range

			uint16_t range_max = GRID_AIN_MAXVALUE - (1<<16-resolution);

			int32_t next = value * (max - min) / range_max + min;

			template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;
    
			grid_ui_smart_trigger(&grid_ui_state, grid_sys_state.bank_activebank_number, res_index, GRID_UI_EVENT_AC);		
			
		}

	}
		
	grid_module_po16_revb_hardware_transfer_complete = 0;
	grid_module_po16_revb_hardware_start_transfer();
}


void grid_module_po16_event_clear_cb(struct grid_ui_event* eve){

}

void grid_module_po16_page_change_cb(uint8_t page_old, uint8_t page_new){

	grid_sys_state.bank_active_changed = 0;
	
	for (uint8_t i=0; i<grid_ui_state.bank_list[grid_sys_state.bank_activebank_number].element_list_length; i++){
		
		grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_INIT);
		grid_ui_smart_trigger_local(&grid_ui_state, grid_sys_state.bank_activebank_number, i, GRID_UI_EVENT_AC);
								
	}
}

void grid_module_po16_revb_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
		
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}

void grid_module_po16_revb_init(){
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5);
	grid_led_lowlevel_init(&grid_led_state, 16);

	grid_ui_model_init(&grid_ui_state, GRID_SYS_BANK_MAXNUMBER);
	
	for(uint8_t i=0; i<GRID_SYS_BANK_MAXNUMBER; i++){	
		
		grid_ui_bank_init(&grid_ui_state, i, 16);
		
		for(uint8_t j=0; j<16; j++){
			
			grid_ui_element_init(&grid_ui_state.bank_list[i], j, GRID_UI_ELEMENT_POTENTIOMETER);

			int32_t* template_parameter_list = grid_ui_state.bank_list[i].element_list[j].template_parameter_list;
	
			template_parameter_list[GRID_LUA_FNC_P_ELEMENT_INDEX_index]		= j;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_NUMBER_index] 	= j;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] 	= 0;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index] 		= 0;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index] 		= 127;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index] 	= 7;
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] 	= 0;

		}
	}
	
	grid_ui_state.event_clear_cb = &grid_module_po16_event_clear_cb;
	grid_ui_state.page_change_cb = &grid_module_po16_page_change_cb;

	grid_module_po16_revb_hardware_init();
	grid_module_po16_revb_hardware_start_transfer();
	
}
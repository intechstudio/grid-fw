#include "grid_module_po16_revb.h"

volatile uint8_t grid_module_po16_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_po16_revb_mux =0;
volatile uint8_t grid_module_po16_revb_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

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

	// POT POLARITY IS REVERSED ON PO16_RevC
	if (grid_sys_get_hwcfg() == GRID_MODULE_PO16_RevC){
		
		// Reverse the 16bit result
		adcresult_0	= 65535 - adcresult_0;
		adcresult_1 = 65535 - adcresult_1;
	}

	grid_ain_add_sample(adc_index_0, adcresult_0);
	grid_ain_add_sample(adc_index_1, adcresult_1);


	uint8_t result_index[2] = {0};
	
	result_index[0] = adc_index_0;
	result_index[1] = adc_index_1;


	uint8_t grid_module_po16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};

	// Process both ADC results

	for (uint8_t i=0; i<2; i++)
	{
	
		// Helper variable for readability
		uint8_t res_index = result_index[i];

		uint32_t* template_parameter_list = grid_ui_state.element[res_index].template_a_parameter_list;
	
		if (grid_ain_get_changed(res_index)){
		
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER] = res_index;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_NUMBER_REVERSED] = grid_module_po16_mux_reversed_lookup[res_index];

			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV7] = grid_ain_get_average(res_index, 7);
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV8] = grid_ain_get_average(res_index, 8);
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14U] = 0;
			template_parameter_list[GRID_TEMPLATE_A_PARAMETER_CONTROLLER_AV14L] = 0;
			
			
			uint8_t event_index = grid_ui_event_find(&grid_ui_state.element[res_index], GRID_UI_EVENT_AVC7);

			grid_ui_event_template_action(&grid_ui_state.element[res_index], event_index);
			
			grid_ui_event_trigger(&grid_ui_state.element[res_index].event_list[event_index]);			
			
		}

	}
	
	
	grid_module_po16_revb_hardware_transfer_complete = 0;
	grid_module_po16_revb_hardware_start_transfer();
}

void grid_module_po16_revb_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
		
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}




void grid_module_po16_revb_init(){
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5, 14, 7);
	grid_led_lowlevel_init(&grid_led_state, 16);

	
	grid_ui_model_init(&grid_ui_state, 16);
	
	for(uint8_t i=0; i<16; i++){
			
		grid_ui_element_init(&grid_ui_state, i, GRID_UI_ELEMENT_POTENTIOMETER);


		uint8_t payload_template[100] = {0};
		sprintf(payload_template, GRID_EVENT_AVC7_POT GRID_DEFAULT_ACTION_AVC7);
		uint8_t payload_length = strlen(payload_template);

		// Register Absolute Value Change
		grid_ui_event_register_action(&grid_ui_state.element[i], GRID_UI_EVENT_AVC7, payload_template, payload_length);
		
	}
	
	grid_module_po16_revb_hardware_init();
	grid_module_po16_revb_hardware_start_transfer();
	
}
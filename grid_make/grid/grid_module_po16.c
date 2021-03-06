#include "grid_module_po16.h"

volatile uint8_t grid_module_po16_hardware_transfer_complete = 0;
volatile uint8_t grid_module_po16_mux =0;
volatile uint8_t grid_module_po16_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
volatile uint8_t grid_module_po16_mux_reversed_lookup[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};


static uint32_t last_real_time[16] = {0};

void grid_module_po16_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);
	
}

static void grid_module_po16_hardware_transfer_complete_cb(void){

	if (grid_module_po16_hardware_transfer_complete == 0){
		grid_module_po16_hardware_transfer_complete++;
		return;
	}
	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_po16_mux_lookup[grid_module_po16_mux+8];
	uint8_t adc_index_1 = grid_module_po16_mux_lookup[grid_module_po16_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_po16_mux++;
	grid_module_po16_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_po16_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_po16_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_po16_mux/4%2);
	
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
		
    uint8_t resolution_0 = grid_ui_state.element_list[adc_index_0].template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];
    uint8_t resolution_1 = grid_ui_state.element_list[adc_index_1].template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

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

		// limit lastrealtime
		uint32_t elapsed_time = grid_sys_rtc_get_elapsed_time(&grid_sys_state, last_real_time[res_index]);
		if (GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS < grid_sys_rtc_get_elapsed_time(&grid_sys_state, last_real_time[res_index])){
			last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state) - GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
			elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT*RTC1MS;
		}

		int32_t* template_parameter_list = grid_ui_state.element_list[res_index].template_parameter_list;

		if (grid_ain_get_changed(res_index)){

			// update lastrealtime
			last_real_time[res_index] = grid_sys_rtc_get_time(&grid_sys_state); 
			template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] = elapsed_time/RTC1MS;

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

			// for display in editor
			int32_t state = value * (127 - 0) / range_max;
   			template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = state;

			struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[res_index], GRID_UI_EVENT_AC);
			grid_ui_event_trigger(eve);	
			
		}

	}
		
	grid_module_po16_hardware_transfer_complete = 0;
	grid_module_po16_hardware_start_transfer();
}



void grid_module_po16_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_hardware_transfer_complete_cb);
		
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}

void grid_module_po16_init(){
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5);
	grid_led_lowlevel_init(&grid_led_state, 16);

	grid_ui_model_init(&grid_ui_state, 16+1); // +1 for the system element
	
	for(uint8_t j=0; j<16; j++){
		
		grid_ui_element_init(&grid_ui_state, j, GRID_UI_ELEMENT_POTENTIOMETER);

	}
	
	grid_module_po16_hardware_init();
	grid_module_po16_hardware_start_transfer();
	
}
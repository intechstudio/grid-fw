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
	

	
	/* Read mapmode state*/
	
	
	struct grid_ui_model* mod = &grid_ui_state;
	
	//CRITICAL_SECTION_ENTER()

	uint8_t report_index = 0;

	uint8_t mapmode_value = gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != mod->report_array[report_index].helper[0]){
		
		uint8_t value;
		
		if (mod->report_array[report_index].helper[0] == 0){
			
			mod->report_array[report_index].helper[0] = 1;
			
			grid_sys_state.bank_select = (grid_sys_state.bank_select+1)%4;
			value = grid_sys_state.bank_select;
			grid_sys_write_hex_string_value(&mod->report_array[report_index].payload[7], 2, grid_sys_state.bank_select);
			grid_ui_report_set_changed_flag(mod, report_index);
		}
		else{
			
			mod->report_array[report_index].helper[0] = 0;
		}
		
		
		

	}

	//CRITICAL_SECTION_LEAVE()


	
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


	grid_ain_add_sample(adc_index_0, adcresult_0);
	grid_ain_add_sample(adc_index_1, adcresult_1);

	
	//CRITICAL_SECTION_ENTER()

	if (grid_ain_get_changed(adc_index_0)){
		uint8_t value = grid_ain_get_average(adc_index_0, 7);	
		uint8_t actuator = 2*value;
		
		
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[7], 2, adc_index_0);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[9], 2, value);
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[21], 2, actuator);
		
		grid_ui_report_set_changed_flag(mod, adc_index_0+1);
		mod->report_array[adc_index_0+1].helper[0] = value;
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	//CRITICAL_SECTION_ENTER()

	if (grid_ain_get_changed(adc_index_1)){
		uint8_t value = grid_ain_get_average(adc_index_1, 7);
		uint8_t actuator = 2*value;
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[7], 2, adc_index_1);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[9], 2, value);
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[21], 2, actuator);
		
		grid_ui_report_set_changed_flag(mod, adc_index_1+1);
		mod->report_array[adc_index_1+1].helper[0] = value;
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	grid_module_po16_revb_hardware_transfer_complete = 0;
	grid_module_po16_revb_hardware_start_transfer();
}

void grid_module_po16_revb_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_po16_revb_hardware_transfer_complete_cb);
		
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}




void grid_module_po16_revb_init(struct grid_ui_model* mod){
	

	grid_ui_model_init(mod, 17);
	
	// 0 is for mapmode_button
	// 1...16 is for ui_buttons
	for(uint8_t i=0; i<17; i++){
		
		uint8_t payload_template[30];
		
		if (i == 0){
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_SYS,
			GRID_MSG_COMMAND_SYS_BANK,
			GRID_MSG_COMMAND_SYS_BANK_SELECT,
			0,
			GRID_MSG_END_OF_TEXT

			);
			
		}
		else{
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_CONTROLCHANGE,
			i-1,
			0,
			GRID_MSG_END_OF_TEXT,
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_LED,
			0, // layer
			GRID_MSG_COMMAND_LED_SET_PHASE,
			i-1,
			0,
			GRID_MSG_END_OF_TEXT

			);
			
		}

		
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[20];
		sprintf(helper_template, "00"); // LASTVALUE
		
		uint8_t helper_length = strlen(helper_template);

		grid_ui_report_init(mod, i, payload_template, payload_length, helper_template, helper_length);
		
	}
	

		
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5, 14, 7);

	grid_led_init(&grid_led_state, 16);
	
	grid_module_po16_revb_hardware_init();
	grid_module_po16_revb_hardware_start_transfer();
	
}
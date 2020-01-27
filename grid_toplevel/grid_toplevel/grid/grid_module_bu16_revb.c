#include "grid_module_bu16_revb.h"

volatile uint8_t grid_module_bu16_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_bu16_revb_mux = 0;
//volatile uint8_t grid_module_bu16_revb_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
	
volatile uint8_t grid_module_bu16_revb_mux_lookup[16] =       {12, 13, 8, 9, 4, 5, 0, 1, 14, 15, 10, 11, 6, 7, 2, 3};


void grid_module_bu16_revb_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);

}

static void grid_module_bu16_revb_hardware_transfer_complete_cb(void){
		
	if (grid_module_bu16_revb_hardware_transfer_complete == 0){
		grid_module_bu16_revb_hardware_transfer_complete++;
		return;
	}
	
		
	struct grid_ui_model* mod = &grid_ui_state;
	

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
	

	
	if (adcresult_0>10000){
		adcresult_0 = 0;
	}
	else{
		adcresult_0 = 127;
	}
	
	if (adcresult_1>10000){
		adcresult_1 = 0;
	}
	else{
		adcresult_1 = 127;
	}
	
	//CRITICAL_SECTION_ENTER()

	if (adcresult_0 != mod->report_ui_array[adc_index_0].helper[0]){
		
		uint8_t command;
		uint8_t velocity;
		
		if (mod->report_ui_array[adc_index_0].helper[0] == 0){
			
			command = GRID_MSG_COMMAND_MIDI_NOTEON;
			velocity = 127;
		}
		else{
			
			command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
		
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[5], 2, command);
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[7], 2, adc_index_0);
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[9], 2, velocity);
		
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[21], 2, actuator);
		mod->report_ui_array[adc_index_0].helper[0] = velocity;
		
		grid_report_ui_set_changed_flag(mod, adc_index_0);
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	//CRITICAL_SECTION_ENTER()

	if (adcresult_1 != mod->report_ui_array[adc_index_1].helper[0]){
		
		uint8_t command;
		uint8_t velocity;
		
		if (mod->report_ui_array[adc_index_1].helper[0] == 0){
			
			command = GRID_MSG_COMMAND_MIDI_NOTEON;
			velocity = 127;
		}
		else{
			
			command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
		
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[5], 2, command);
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[7], 2, adc_index_1);
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[9], 2, velocity);
		
		grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[21], 2, actuator);
		
		mod->report_ui_array[adc_index_1].helper[0] = velocity;
		
		grid_report_ui_set_changed_flag(mod, adc_index_1);
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	grid_module_bu16_revb_hardware_transfer_complete = 0;
	grid_module_bu16_revb_hardware_start_transfer();
}

void grid_module_bu16_revb_hardware_init(void){
	

	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}



void grid_module_bu16_revb_init(struct grid_ui_model* mod){

	grid_led_init(&grid_led_state, 16);
	grid_ui_model_init(mod, 16);
		
	for(uint8_t i=0; i<16; i++){
		
		uint8_t payload_template[30];
			
		uint8_t grid_module_bu16_revb_mux_lookup_led[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
		sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c%c%02x%02x%02x%02x%02x%c",
			
		GRID_MSG_START_OF_TEXT,
		GRID_MSG_PROTOCOL_MIDI,
		0, // (cable<<4) + channel
		GRID_MSG_COMMAND_MIDI_NOTEON,
		i,
		0,
		GRID_MSG_END_OF_TEXT,
			
		GRID_MSG_START_OF_TEXT,
		GRID_MSG_PROTOCOL_LED,
		0, // layer
		GRID_MSG_COMMAND_LED_SET_PHASE,
		grid_module_bu16_revb_mux_lookup_led[i],
		0,
		GRID_MSG_END_OF_TEXT

		);
			
		
		
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;
		
		uint8_t error = grid_report_ui_init(mod, i, payload_template, payload_length, helper_template, helper_length);
		

	}
	
	grid_report_sys_init(mod);
			
	grid_module_bu16_revb_hardware_init();
	grid_module_bu16_revb_hardware_start_transfer();

};

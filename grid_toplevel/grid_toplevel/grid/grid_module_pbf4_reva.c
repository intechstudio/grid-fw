#include "grid_module_pbf4_reva.h"

volatile uint8_t grid_module_pbf4_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_pbf4_revb_mux =0;
volatile uint8_t grid_module_pbf4_reva_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

void grid_module_pbf4_reva_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);
	
}

void grid_module_pbf4_reva_hardware_transfer_complete_cb(void){
	
	if (grid_module_pbf4_reva_hardware_transfer_complete == 0){
		grid_module_pbf4_reva_hardware_transfer_complete++;
		return;
	}
	
	
	
	struct grid_ui_model* mod = &grid_ui_state;
	
	
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
		
		uint8_t adcresult_0_valid = 0;
	
		if (adcresult_0>60000){
			adcresult_0 = 0;
			adcresult_0_valid = 1;
		}
		else if (adcresult_0<200){
			adcresult_0 = 127;
			adcresult_0_valid = 1;
		}
		
		uint8_t adcresult_1_valid = 0;
	
		if (adcresult_1>60000){
			adcresult_1 = 0;
			adcresult_1_valid = 1;
		}
		else if (adcresult_1<200){
			adcresult_1 = 127;
			adcresult_1_valid = 1;
		}
		
		
		//CRITICAL_SECTION_ENTER()

		if (adcresult_0 != mod->report_ui_array[adc_index_0-4].helper[0] && adcresult_0_valid){
			
			uint8_t command;
			uint8_t velocity;
			
			if (mod->report_ui_array[adc_index_0-4].helper[0] == 0){
				
				command = GRID_MSG_COMMAND_MIDI_NOTEON;
				velocity = 127;
			}
			else{
				
				command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
				velocity = 0;
			}
			
			uint8_t actuator = 2*velocity;
			
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0-4].payload[5], 2, command);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0-4].payload[7], 2, adc_index_0);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0-4].payload[9], 2, velocity);
			mod->report_ui_array[adc_index_0-4].helper[0] = velocity;
			grid_report_ui_set_changed_flag(mod, adc_index_0-4);
				
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0-4+12].payload[9], 2, actuator);
			grid_report_ui_set_changed_flag(mod, adc_index_0-4+12);
		}
		
		//CRITICAL_SECTION_LEAVE()
		
		//CRITICAL_SECTION_ENTER()

		if (adcresult_1 != mod->report_ui_array[adc_index_1-4].helper[0] && adcresult_1_valid){
			
			uint8_t command;
			uint8_t velocity;
			
			if (mod->report_ui_array[adc_index_1-4].helper[0] == 0){
				
				command = GRID_MSG_COMMAND_MIDI_NOTEON;
				velocity = 127;
			}
			else{
				
				command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
				velocity = 0;
			}
			
			uint8_t actuator = 2*velocity;
			
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1-4].payload[5], 2, command);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1-4].payload[7], 2, adc_index_1);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1-4].payload[9], 2, velocity);		
			mod->report_ui_array[adc_index_1-4].helper[0] = velocity;		
			grid_report_ui_set_changed_flag(mod, adc_index_1-4);
				
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1-4+12].payload[9], 2, actuator);		
			grid_report_ui_set_changed_flag(mod, adc_index_1-4+12);
			
		}
		
		//CRITICAL_SECTION_LEAVE()

	}
	else{ // POTENTIOMETER OR FADER
		
		if (adc_index_1 == 0 || adc_index_1 == 1){
			
			grid_ain_add_sample(adc_index_0, (1<<16)-1-adcresult_0);
			grid_ain_add_sample(adc_index_1, (1<<16)-1-adcresult_1);
			
		}
		else{
						
			grid_ain_add_sample(adc_index_0, adcresult_0);
			grid_ain_add_sample(adc_index_1, adcresult_1);
			
		}
			
			
		
		
		//CRITICAL_SECTION_ENTER()

		if (grid_ain_get_changed(adc_index_0)){

			uint8_t value = grid_ain_get_average(adc_index_0, 7);
			uint8_t actuator = 2*value;
		
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[7], 2, adc_index_0);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0].payload[9], 2, value);
			grid_report_ui_set_changed_flag(mod, adc_index_0);	
			
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_0 + 12].payload[9], 2, actuator);			
			grid_report_ui_set_changed_flag(mod, adc_index_0 + 12);
		}
	
		//CRITICAL_SECTION_LEAVE()
	
	
		//CRITICAL_SECTION_ENTER()

		if (grid_ain_get_changed(adc_index_1)){

			uint8_t value = grid_ain_get_average(adc_index_1, 7);
			uint8_t actuator = 2*value;
		
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[7], 2, adc_index_1);
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1].payload[9], 2, value);
			grid_report_ui_set_changed_flag(mod, adc_index_1);
			
			grid_sys_write_hex_string_value(&mod->report_ui_array[adc_index_1 + 12].payload[9], 2, actuator);		
			grid_report_ui_set_changed_flag(mod, adc_index_1 + 12);
		}
	
		//CRITICAL_SECTION_LEAVE()
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




void grid_module_pbf4_reva_init(struct grid_ui_model* mod){
	
	
	// 16 pot, depth of 5, 14bit internal, 7bit result;
	grid_ain_init(16, 5, 14, 7);

	grid_led_init(&grid_led_state, 12);
	
	grid_ui_model_init(mod, 24);
	
	for(uint8_t i=0; i<24; i++){
		
		uint8_t payload_template[30] = {0};
		enum grid_report_type_t type;
		
		if (i<8){ // PORENTIOMETERS & FADERS -> MIDI Control Change
			
			type = GRID_REPORT_TYPE_BROADCAST;
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_CONTROLCHANGE,
			i,
			0,
			GRID_MSG_END_OF_TEXT

			);
			
		}
		else if (i<12){ // BUTTONS -> MIDI Note On/Off
			
			type = GRID_REPORT_TYPE_BROADCAST;
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
						
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_NOTEON,
			i+4,
			0,
			
			GRID_MSG_END_OF_TEXT
			);
									
		}
		else{ // LED -> Grid LED
			
			type = GRID_REPORT_TYPE_LOCAL;
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_LED,
			0, // layer
			GRID_MSG_COMMAND_LED_SET_PHASE,
			i-12,
			0,
			GRID_MSG_END_OF_TEXT

			);			
		}

		
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;
		
		grid_report_ui_init(mod, i, type, payload_template, payload_length, helper_template, helper_length);
		
	}
	
	grid_report_sys_init(mod);
		
	grid_module_pbf4_reva_hardware_init();
	grid_module_pbf4_reva_hardware_start_transfer();
	
}
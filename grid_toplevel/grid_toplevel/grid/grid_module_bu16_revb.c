#include "grid_module_bu16_revb.h"


static const uint8_t grid_module_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

static void grid_module_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);
	
}

static void grid_module_hardware_transfer_complete_cb(void){
	
	if (grid_ui_button_hardware_transfer_complete == 0){
		grid_ui_button_hardware_transfer_complete++;
		return;
	}
	
	
	/* Read mapmode state*/
	
	
	struct grid_ui_model* mod = &grid_ui_state;
	
	CRITICAL_SECTION_ENTER()

	uint8_t report_index = 0;

	uint8_t mapmode_value = gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != mod->report_array[report_index].helper[0]){
		
		uint8_t command;
		
		if (mod->report_array[report_index].helper[0] == 0){
			
			command = GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYDOWN;
			mod->report_array[report_index].helper[0] = 1;
		}
		else{
			
			command = GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYUP;
			mod->report_array[report_index].helper[0] = 0;
		}
		
		
		
		grid_sys_write_hex_string_value(&mod->report_array[report_index].payload[3], 2, command);
		
		grid_ui_report_set_changed_flag(mod, report_index);
	}

	CRITICAL_SECTION_LEAVE()


	
	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_mux_lookup[grid_module_mux+8];
	uint8_t adc_index_1 = grid_module_mux_lookup[grid_module_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_mux++;
	grid_module_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_mux/4%2);
	
	
	
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
	
	CRITICAL_SECTION_ENTER()

	if (adcresult_0 != mod->report_array[adc_index_0+1].helper[0]){
		
		uint8_t command;
		uint8_t velocity;
		
		if (mod->report_array[adc_index_0+1].helper[0] == 0){
			
			command = GRID_MSG_COMMAND_MIDI_NOTEON;
			velocity = 127;
		}
		else{
			
			command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[5], 2, command);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[7], 2, adc_index_0);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[9], 2, velocity);
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_0+1].payload[21], 2, actuator);
		mod->report_array[adc_index_0+1].helper[0] = velocity;
		
		grid_ui_report_set_changed_flag(mod, adc_index_0+1);
	}
	
	CRITICAL_SECTION_LEAVE()
	
	
	CRITICAL_SECTION_ENTER()

	if (adcresult_1 != mod->report_array[adc_index_1+1].helper[0]){
		
		uint8_t command;
		uint8_t velocity;
		
		if (mod->report_array[adc_index_1+1].helper[0] == 0){
			
			command = GRID_MSG_COMMAND_MIDI_NOTEON;
			velocity = 127;
		}
		else{
			
			command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[5], 2, command);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[7], 2, adc_index_1);
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[9], 2, velocity);
		
		grid_sys_write_hex_string_value(&mod->report_array[adc_index_1+1].payload[21], 2, actuator);
		
		mod->report_array[adc_index_1+1].helper[0] = velocity;
		
		grid_ui_report_set_changed_flag(mod, adc_index_1+1);
	}
	
	CRITICAL_SECTION_LEAVE()
	
	
	grid_ui_button_hardware_transfer_complete = 0;
	grid_module_hardware_start_transfer();
}

static void grid_module_hardware_init(void){
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}



void grid_module_bu16_revb_init(struct grid_ui_model* mod){
	
	mod->report_length = 17;
	mod->report_array = malloc(mod->report_length*sizeof(struct grid_ui_report));
	
	
	// 0 is for mapmode_button
	// 1...16 is for ui_buttons
	for(uint8_t i=0; i<17; i++){
		
		uint8_t payload_template[30];
		
		if (i == 0){
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%c%",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_KEYBOARD,
			GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYDOWN,
			GRID_MSG_PROTOCOL_KEYBOARD_PARAMETER_NOT_MODIFIER,
			HID_CAPS_LOCK,
			GRID_MSG_END_OF_TEXT

			);
			
		}
		else{
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_NOTEON,
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

	grid_led_init(&grid_led_state, 16);
	grid_module_init_animation(&grid_led_state);
	
	grid_module_hardware_init();
	grid_module_hardware_start_transfer();
	
}

#include "grid_module_en16_reva.h"



struct grid_ui_encoder grid_ui_encoder_array[16];

static uint8_t UI_SPI_TX_BUFFER[14] = "aaaaaaaaaaaaaa";
static uint8_t UI_SPI_RX_BUFFER[14];
static uint8_t UI_SPI_TRANSFER_LENGTH = 10;

static uint8_t UI_SPI_DEBUG = 8;

volatile uint8_t UI_SPI_DONE = 0;


volatile uint8_t UI_SPI_RX_BUFFER_LAST[16];

static uint8_t UI_ENCODER_BUTTON_STATE[16];
static uint8_t UI_ENCODER_BUTTON_STATE_CHANGED[16];

static uint8_t UI_ENCODER_ROTATION_STATE[16];
static uint8_t UI_ENCODER_ROTATION_STATE_CHANGED[16];


static uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;










static void grid_module_hardware_start_transfer(void){
	

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);

	spi_m_async_enable(&UI_SPI);

	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

static void grid_module_hardware_transfer_complete_cb(void){
	
	/* Transfer completed */

	struct grid_ui_model* mod = &grid_ui_state;
	
	

	grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_MASTER);
	grid_sync_set_level(GRID_SYNC_1, 1);
		
	grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_MASTER);

	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

		
	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t i=0; i<16; i++){

		uint8_t new_value = (UI_SPI_RX_BUFFER[i/2]>>(4*(i%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[i];
			
		if (old_value != new_value){

				
			UI_SPI_DEBUG = i;
				
			uint8_t button_value = new_value>>2;
			uint8_t phase_a = (new_value>>1)&1;
			uint8_t phase_b = (new_value)&1;
				
			if (button_value != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_value){
				// BUTTON CHANGE
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_changed = 1;
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_value = new_value>>2;
				
				
				CRITICAL_SECTION_ENTER()
					
				uint8_t command;
				uint8_t velocity;
				
				if (mod->report_array[UI_ENCODER_LOOKUP[i]+1].helper[0] == 0){
					
					command = GRID_MSG_COMMAND_MIDI_NOTEON;
					velocity = 127;
				}
				else{
					
					command = GRID_MSG_COMMAND_MIDI_NOTEOFF;
					velocity = 0;
				}
				
				uint8_t actuator = 2*velocity;
				
				grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1].payload[5], 2, command);
				grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1].payload[7], 2, UI_ENCODER_LOOKUP[i]);
				grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1].payload[9], 2, velocity);
				
				grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1].payload[21], 2, actuator);
				mod->report_array[UI_ENCODER_LOOKUP[i]+1].helper[0] = velocity;
				
				grid_ui_report_set_changed_flag(mod, UI_ENCODER_LOOKUP[i]+1);
				
				
				CRITICAL_SECTION_LEAVE()
				
				
			}
				
				
			if (phase_a != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_a_previous){
					
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_a_previous = phase_a;
					
				if (phase_b == 0){
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = phase_a;
				}
				else{
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = !phase_a;
				}
					
				if (phase_a && phase_b){
						
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value += grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction*2 -1;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value %= 128;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_changed = 1;
					
					
					CRITICAL_SECTION_ENTER()
					
					uint8_t command = GRID_MSG_COMMAND_MIDI_CONTROLCHANGE;
					uint8_t value = grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value;
					uint8_t actuator = 2*value;
					
					if (value != mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].helper[0]){
					
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[5], 2, command);
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[7], 2, UI_ENCODER_LOOKUP[i]);
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[9], 2, value);
						
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[21], 2, actuator);
						mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].helper[0] = value;
						
						grid_ui_report_set_changed_flag(mod, UI_ENCODER_LOOKUP[i]+1+16);
						
					}
	
					CRITICAL_SECTION_LEAVE()
				}
			}
				
			if (phase_b != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_b_previous){
					
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_b_previous = phase_b;
					
				if (phase_a == 0){
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = !phase_b;
				}
				else{
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = phase_b;
				}
					
				if (phase_a && phase_b){

					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value += grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction*2 -1;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value %= 128;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_changed = 1;
					
										CRITICAL_SECTION_ENTER()
					
					uint8_t command = GRID_MSG_COMMAND_MIDI_CONTROLCHANGE;
					uint8_t value = grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value;
					uint8_t actuator = 2*value;
					
					if (value != mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].helper[0]){
					
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[5], 2, command);
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[7], 2, UI_ENCODER_LOOKUP[i]);
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[9], 2, value);
						
						grid_sys_write_hex_string_value(&mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].payload[21], 2, actuator);
						mod->report_array[UI_ENCODER_LOOKUP[i]+1+16].helper[0] = value;
						
						grid_ui_report_set_changed_flag(mod, UI_ENCODER_LOOKUP[i]+1+16);
						
					}
	
					CRITICAL_SECTION_LEAVE()
				}
			}
				

				
		}
		else{
				
		}
			
	}
		
	
	
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

		
	
	grid_sync_set_level(GRID_SYNC_1, 0);

	grid_module_hardware_transfer_complete = 0;
	grid_module_hardware_start_transfer();
}

static void grid_module_hardware_init(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);
	
	
	
	
	
	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
	
	spi_m_async_get_io_descriptor(&UI_SPI, &grid_hardware_io);


	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_module_hardware_transfer_complete_cb);
	

}



void grid_module_en16_reva_init(struct grid_ui_model* mod){
	
	mod->report_length = 1+16+16;
	mod->report_array = malloc(mod->report_length*sizeof(struct grid_ui_report));
	
	
	// 0 is for mapmode_button
	// 1...16 is for ui_buttons
	for(uint8_t i=0; i<1+16+16; i++){
		
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
		else if (i<1+16){
			
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
		else{
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%02x%c%c%02x%02x%02x%02x%02x%c",
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_CONTROLCHANGE,
			i-1-16,
			0,
			GRID_MSG_END_OF_TEXT,
			
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_LED,
			0, // layer
			GRID_MSG_COMMAND_LED_SET_PHASE,
			i-1-16,
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

	for (uint8_t i = 0; i<16; i++)
	{
		grid_ui_encoder_array[i].controller_number = i;
	}
	
	
	grid_led_init(&grid_led_state, 16);
	grid_module_init_animation(&grid_led_state);
	
	grid_module_hardware_init();
	grid_module_hardware_start_transfer();
	
}

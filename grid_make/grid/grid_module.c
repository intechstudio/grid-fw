#include "grid_module.h"



uint8_t grid_sync_mode_register[2] = {0, 0};

//====================== GRID ENCODER K+F ===================================//



	
	
void grid_sync_set_mode(enum grid_sync_selector sync_select, enum grid_sync_mode sync_mode){
	
	grid_sync_mode_register[sync_select - 1]  = sync_mode;
	
	if (sync_select == GRID_SYNC_1){		
		
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){	
			
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	
}

enum grid_sync_mode grid_sync_get_mode(enum grid_sync_selector sync_select){
	
	if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_MASTER){
		return GRID_SYNC_MASTER;
	}
	else if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_SLAVE){
		return GRID_SYNC_SLAVE;
	}
	else{
		return GRID_SYNC_INITIAL;	
	}	
}

void grid_sync_set_level(enum grid_sync_selector sync_select, uint8_t sync_level){
	
	if (sync_select == GRID_SYNC_1){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, sync_level);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, sync_level);
		}
		
	}
		
}



	





	
#define GRID_ADC_CFG_REVERSED	0
#define GRID_ADC_CFG_BINARY		1

	
uint8_t grid_adc_cfg[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	
	
void grid_adc_set_config(uint8_t register_offset, uint8_t bit_offest, uint8_t value){
		
	if (value){
		grid_adc_cfg[register_offset] |= (1<<bit_offest);
	}
	else{
		grid_adc_cfg[register_offset] &= ~(1<<bit_offest);
	}
		
}
	
void grid_adc_set_config_default(uint8_t register_offset){
		
	grid_adc_cfg[register_offset] = 0;
}	
	
uint8_t grid_adc_get_config(uint8_t register_offset, uint8_t bit_offest){
		
	return (grid_adc_cfg[register_offset] & (1<<bit_offest));
		
}	
		
		
	

// Define all of the peripheral interrupt callbacks
	



	
/* ============================== GRID_MODULE_INIT() ================================ */


void grid_module_common_init(void){
	
	printf("Common init done\r\n");	

	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);

	// set priorities for all of the UI related interrupts

	grid_d51_nvic_set_interrupt_priority(ADC0_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC0_1_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC1_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC1_1_IRQn, 1); 

	grid_d51_nvic_set_interrupt_priority(SERCOM3_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(SERCOM3_1_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(SERCOM3_2_IRQn, 1); // SERCOM3_2_IRQn handles reading encoders
	grid_d51_nvic_set_interrupt_priority(SERCOM3_3_IRQn, 1); 

	// disable ui interrupts
	grid_d51_nvic_set_interrupt_priority_mask(1);

	
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevC){
		printf("Init Module: PO16\r\n");
		grid_module_po16_init();
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevC ){
		printf("Init Module: BU16\r\n");
		grid_module_bu16_init();
	
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevA){
		printf("Init Module: PBF4\r\n");					
		grid_module_pbf4_init();			
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD ){
		printf("Init Module: EN16\r\n");
		grid_module_en16_init();	
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ){
		printf("Init Module: EN16 ND\r\n");
		grid_module_en16_init();	
	}		
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD ){
		printf("Init Module: EF44\r\n");
		grid_module_ef44_init();	
	}	
	else{
		printf("Init Module: Unknown Module\r\n");
	}

	printf("Model init done\r\n");


	grid_ui_element_init(&grid_ui_state, grid_ui_state.element_list_length-1, GRID_UI_ELEMENT_SYSTEM);
	


	grid_port_init_all();
	grid_sys_uart_init();
	grid_sys_dma_rx_init();

	grid_sys_set_bank(&grid_sys_state, 0);

	grid_nvm_init(&grid_nvm_state, &FLASH_0);
	
		
}



void grid_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf){

	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;


	template_parameter_list[GRID_LUA_FNC_P_ELEMENT_INDEX_index]		= element_index;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index] 	= 7;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] 	= 0;

	// Load AIN value to VALUE register

	int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

	if (resolution < 1){
		resolution = 1;
	}
	else if (resolution > 12){
		resolution = 12;
	}

	int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
	int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

	int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, 16, resolution, min, max);
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;


	

}

void grid_element_button_template_parameter_init(struct grid_ui_template_buffer* buf){
	
	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;

	template_parameter_list[GRID_LUA_FNC_B_ELEMENT_INDEX_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] 		= 0;
}

void grid_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf){

	//printf("template parameter init\r\n");

	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;

	template_parameter_list[GRID_LUA_FNC_E_ELEMENT_INDEX_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] 		= 0;

	template_parameter_list[GRID_LUA_FNC_E_ENCODER_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index] 		= 128 - 1;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] 	= 64;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_VELOCITY_index] 	= 50;

}



void grid_element_button_event_clear_cb(struct grid_ui_event* eve){


}

void grid_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){

	
	// for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){

	// 	struct grid_ui_event* eve = NULL;

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_INIT);
	// 	grid_ui_event_trigger_local(eve);	

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
	// 	grid_ui_event_trigger_local(eve);	
	// }
}



void grid_element_encoder_event_clear_cb(struct grid_ui_event* eve){


	int32_t* template_parameter_list = eve->parent->template_parameter_list;



	template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] = 64;

	if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 1){ // relative

		int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
		int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];

		template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = ((max+1)-min)/2;

	}	
	else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 2){

		template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = 0;

	}
 
}

void grid_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){

			
	// for (uint8_t i = 0; i<16; i++)
	// {
		
	// 	struct grid_ui_event* eve = NULL;

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_INIT);
	// 	grid_ui_event_trigger_local(eve);	

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_EC);
	// 	grid_ui_event_trigger_local(eve);	
	// }

}





void grid_element_potmeter_event_clear_cb(struct grid_ui_event* eve){

}

void grid_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){


	uint8_t element_index = ele->index;
	int32_t* template_parameter_list = ele->template_parameter_list;


	int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

	if (resolution < 1){
		resolution = 1;
	}
	else if (resolution > 12){
		resolution = 12;
	}

	int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
	int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

	int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, 16, resolution, min, max);
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;
}
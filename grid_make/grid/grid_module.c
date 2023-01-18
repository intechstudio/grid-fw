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
	grid_d51_uart_init();

	grid_sys_set_bank(&grid_sys_state, 0);

	grid_d51_nvm_init(&grid_d51_nvm_state, &FLASH_0);
	
		
}



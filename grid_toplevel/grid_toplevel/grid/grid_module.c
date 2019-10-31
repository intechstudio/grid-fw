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
	



void grid_module_init_animation(struct grid_led_model* mod){
	

	
	for (uint8_t i = 0; i<255; i++){
			
		// SEND DATA TO LEDs
			
			
		uint8_t color_r   = i;
		uint8_t color_g   = i;
		uint8_t color_b   = i;
			
				
		for (uint8_t i=0; i<mod->led_number; i++){
			//grid_led_set_color(i, 0, 255, 0);
			grid_led_set_color(&grid_led_state, i, color_r, color_g, color_b);
				
				
		}
			
			
		grid_led_hardware_start_transfer_blocking(&grid_led_state);
			
			

				
		delay_ms(1);
		
			
	}
	

	

	
}


	
/* ============================== GRID_MODULE_INIT() ================================ */


void grid_module_common_init(void){
				

	
	
	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);

	// ADC SETUP	
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_P16_RevB){					
		grid_module_po16_revb_init(&grid_ui_state);	
	}	
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_B16_RevB){	
		grid_module_bu16_revb_init(&grid_ui_state);
	
	}	
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_PBF4_RevA){						
		grid_module_pbf4_reva_init(&grid_ui_state);			
	}
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_EN16_RevA){	
		grid_module_en16_reva_init(&grid_ui_state);
		
	}	
	

	

	grid_port_init_all();
	grid_sys_uart_init();
	grid_rx_dma_init();

	
		
}

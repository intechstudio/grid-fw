/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_sys.h"

uint32_t grid_sys_get_id(uint32_t* return_array){
			
	return_array[0] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_0);
	return_array[1] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_1);
	return_array[2] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_2);
	return_array[3] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_3);
	
	return 1;
	
}

uint32_t grid_sys_get_hwcfg(){
	
	gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OUT);
	gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OUT);
	gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_IN);
	
	// LOAD DATA
	gpio_set_pin_level(HWCFG_SHIFT, 0);
	delay_ms(1);
	
	
	
	uint8_t hwcfg_value = 0;
	
	
	for(uint8_t i = 0; i<8; i++){ // now we need to shift in the remaining 7 values
		
		// SHIFT DATA
		gpio_set_pin_level(HWCFG_SHIFT, 1); //This outputs the first value to HWCFG_DATA
		delay_ms(1);
		
		
		if(gpio_get_pin_level(HWCFG_DATA)){
			
			hwcfg_value |= (1<<i);
			
			}else{
			
			
		}
		
		if(i!=7){
			
			// Clock rise
			gpio_set_pin_level(HWCFG_CLOCK, 1);
			
			delay_ms(1);
			
			gpio_set_pin_level(HWCFG_CLOCK, 0);
		}
		
		
		
		
	}
	
	return hwcfg_value;

}
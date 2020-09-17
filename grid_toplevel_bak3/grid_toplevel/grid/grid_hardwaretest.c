/*
 * grid_hardwaretest.c
 *
 * Created: 2/20/2020 3:51:20 PM
 *  Author: WPC-User
 */ 

#include "grid_hardwaretest.h"

void grid_hardwaretest_main(){
	
	printf("Hardware Test Init");
	
	grid_hardwaretest_led_test_init(&grid_led_state, 16);
	
	uint32_t counter = 0;
	
	uint8_t testmode = 1;
	uint8_t button_last = 1;
	uint8_t button_now = 1;
	uint8_t mode_changed = 1;
	
	while(1){
	
		if (gpio_get_pin_level(MAP_MODE) == 0){
			if (button_last == 1){
				testmode++;
				testmode%=2;
				mode_changed = 1;
				button_last=0;
			}			
		}
		else{
			button_last = 1;
		}
		

	
		if (testmode == 0){
			
			if (mode_changed){
				grid_hardwaretest_led_test_photo(&grid_led_state, counter);
				grid_hardwaretest_led_test_photo(&grid_led_state, counter);
				
				for (uint8_t i=0; i<grid_sys_get_hwcfg()/4; i++){
					
					grid_hardwaretest_led_test_photo(&grid_led_state, counter);
					grid_hardwaretest_led_test_photo(&grid_led_state, counter);
					
				}		
				
			}
			
			
		}
		else if (testmode == 1){
		
			grid_hardwaretest_port_test(counter);
			grid_hardwaretest_led_test(&grid_led_state, counter);	
			
		}
		
		
		delay_ms(1);
		
		mode_changed = 0;	
		counter++;			
		
	}

}

void grid_hardwaretest_led_test_init(struct grid_led_model* mod, uint8_t num){
	
	gpio_set_pin_level(UI_PWR_EN, true);
	
	grid_led_lowlevel_init(mod, num);
	
	for(uint8_t i=0; i<num; i++){
		
		grid_led_lowlevel_set_color(mod, i, 0, 0, 0);
		
	}

	
}


void grid_hardwaretest_led_test(struct grid_led_model* mod, uint32_t loop){
		
		
	for(uint8_t i=0; i<mod->led_number; i++){
	
		grid_led_lowlevel_set_color(mod, i, loop/10%128*(loop/1280%3==0), loop/10%128*(loop/1280%3==1), loop/10%128*(loop/1280%3==2));
	
	}
		
		
	//grid_led_render_all(mod);
		
		
	while(grid_led_lowlevel_hardware_is_transfer_completed(mod) != 1){
			
	}
	grid_led_lowlevel_hardware_start_transfer(mod);
	
}

void grid_hardwaretest_led_test_photo(struct grid_led_model* mod, uint32_t loop){

	uint8_t color_r[4] = {255, 127, 255, 0};
	uint8_t color_g[4] = {0, 255, 127, 127};
	uint8_t color_b[4] = {127, 127, 0, 255};
	
	
	for(uint8_t i=0; i<mod->led_number; i++){
		
		uint8_t intensity = (rand()%255)*(rand()%255)/256.0/2;
		//uint8_t intensity = (rand()%255)/2;
		
		if (intensity<5){
			intensity = 5;
		}
		if (intensity>250){
			intensity = 250;
		}
		
		uint8_t group = (i+4)%4;
		
		grid_led_lowlevel_set_color(mod, i, intensity/256.0*color_r[group], intensity/256.0*color_g[group], intensity/256.0*color_b[group]);

	}
	
	
	//grid_led_render_all(mod);
	
	
	while(grid_led_lowlevel_hardware_is_transfer_completed(mod) != 1){
		
	}
	grid_led_lowlevel_hardware_start_transfer(mod);
	
}


void grid_hardwaretest_port_test(uint32_t loop){

	//INIT PORTS
	
	// SYNC
	gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PIN_GRID_SYNC_1, GPIO_PIN_FUNCTION_OFF);
			
	gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PIN_GRID_SYNC_2, GPIO_PIN_FUNCTION_OFF);
			
	//NORTH
	gpio_set_pin_direction(PC27, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC27, GPIO_PIN_FUNCTION_OFF);
			
	gpio_set_pin_direction(PC28, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC28, GPIO_PIN_FUNCTION_OFF);
			
	//EAST
	gpio_set_pin_direction(PC17, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC17, GPIO_PIN_FUNCTION_OFF);
			
	gpio_set_pin_direction(PC16, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC16, GPIO_PIN_FUNCTION_OFF);
			
	//SOUTH
	gpio_set_pin_direction(PC13, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC13, GPIO_PIN_FUNCTION_OFF);
			
	gpio_set_pin_direction(PC12, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PC12, GPIO_PIN_FUNCTION_OFF);
			
	//WEST
	gpio_set_pin_direction(PB08, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PB08, GPIO_PIN_FUNCTION_OFF);
			
	gpio_set_pin_direction(PB09, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(PB09, GPIO_PIN_FUNCTION_OFF);
			

		
	if (loop%1000 == 0){
			
		gpio_set_pin_level(PIN_GRID_SYNC_1, false);
		gpio_set_pin_level(PIN_GRID_SYNC_2, false);
			
		gpio_set_pin_level(PC27, false);
		gpio_set_pin_level(PC28, false);
			
		gpio_set_pin_level(PC17, false);
		gpio_set_pin_level(PC16, false);
			
		gpio_set_pin_level(PC13, false);
		gpio_set_pin_level(PC12, false);
			
		gpio_set_pin_level(PB08, false);
		gpio_set_pin_level(PB09, false);
			
	}
	if (loop%1000 == 250){
			
		gpio_set_pin_level(PIN_GRID_SYNC_1, false);
		gpio_set_pin_level(PIN_GRID_SYNC_2, false);
			
		gpio_set_pin_level(PC27, true);
		gpio_set_pin_level(PC28, true);
			
		gpio_set_pin_level(PC17, true);
		gpio_set_pin_level(PC16, true);
			
		gpio_set_pin_level(PC13, true);
		gpio_set_pin_level(PC12, true);
			
		gpio_set_pin_level(PB08, true);
		gpio_set_pin_level(PB09, true);
			
	}
	if (loop%1000 == 500){
			
		gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		gpio_set_pin_level(PIN_GRID_SYNC_2, true);
			
		gpio_set_pin_level(PC27, true);
		gpio_set_pin_level(PC28, true);
			
		gpio_set_pin_level(PC17, true);
		gpio_set_pin_level(PC16, true);
			
		gpio_set_pin_level(PC13, true);
		gpio_set_pin_level(PC12, true);
			
		gpio_set_pin_level(PB08, true);
		gpio_set_pin_level(PB09, true);
			
	}
	if (loop%1000 == 750){
			
		gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		gpio_set_pin_level(PIN_GRID_SYNC_2, true);
			
		gpio_set_pin_level(PC27, false);
		gpio_set_pin_level(PC28, false);
			
		gpio_set_pin_level(PC17, false);
		gpio_set_pin_level(PC16, false);
			
		gpio_set_pin_level(PC13, false);
		gpio_set_pin_level(PC12, false);
			
		gpio_set_pin_level(PB08, false);
		gpio_set_pin_level(PB09, false);
			
	}
		

	
	
}

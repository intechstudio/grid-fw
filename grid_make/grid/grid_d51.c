/*
 * grid_d51.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */ 

#include "grid_d51.h"


void grid_d51_bitmap_write_bit(uint8_t* buffer, uint8_t offset, uint8_t value, uint8_t* changed){
	
	uint8_t index = offset/8;
	uint8_t bit = offset%8;
	
	if (value){ // SET BIT

		if ((buffer[index] & (1<<bit)) == 0){
			
			buffer[index] |= (1<<bit);
			*changed = 1;
		}
		else{
			
			// no change needed
		}
		
		
		}else{ // CLEAR BIT
		
		if ((buffer[index] & (1<<bit)) == (1<<bit)){
			
			buffer[index] &= ~(1<<bit);
			*changed = 1;
		}
		else{
			
			// no change needed
		}
		
		
		
	}
	
	
}



void grid_d51_init(){
	
	uint32_t hwid = grid_sys_get_hwcfg();
	
	printf("{\"type\":\"HWCFG\", \"data\": \"%d\"}\r\n", hwid);
	
	
	#ifdef NDEBUG		
	GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_BOOT, "USER ROW CHECK!");
	grid_d51_verify_user_row();
	#else
	GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_BOOT, "NO USER ROW CHECK!");
	#endif
	

			
	#ifdef UNITTEST
	#include "grid/grid_unittest.h"
	grid_unittest_start();
	
	grid_sys_unittest();
	grid_sys_unittest();
	
	printf(" Unit Test Finished\r\n");
	
	while (1)
	{
	}
	
	#else
	
	GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_BOOT, "No Unit Test");
	#endif
	
	//#define HARDWARETEST

	#ifdef HARDWARETEST
	
	#include "grid/grid_hardwaretest.h"

	grid_nvm_init(&grid_nvm_state, &FLASH_0);

    
	grid_hardwaretest_main();
	
	
	while (1)
	{
	}
	#else
	
	GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_BOOT, "No Hardware Test");
	#endif
		
}

void grid_d51_verify_user_row(){
	
	
	uint8_t user_area_buffer[512];
	uint8_t user_area_changed_flag = 0;
		
		
	_user_area_read(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);
		


	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Reading User Row");
	_user_area_read(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);


	//BOD33 characteristics datasheet page 1796

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Verifying User Row");
		
	// BOD33 Disable Bit => Set 0
	grid_d51_bitmap_write_bit(user_area_buffer, 0, 0, &user_area_changed_flag);
		
	// BOD33 Level => Set 225 = b11100001
	grid_d51_bitmap_write_bit(user_area_buffer, 1, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 2, 0, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 3, 0, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 4, 0, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 5, 0, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 6, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 7, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 8, 1, &user_area_changed_flag);
		
	// BOD33 Action => Reset = b01
	grid_d51_bitmap_write_bit(user_area_buffer, 9, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 10, 0, &user_area_changed_flag);

	// BOD33 Hysteresis => Set 15 = b1111
	grid_d51_bitmap_write_bit(user_area_buffer, 11, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 12, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 13, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 14, 1, &user_area_changed_flag);
		
	// BOOTPROTECT 16kB
	grid_d51_bitmap_write_bit(user_area_buffer, 26, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 27, 0, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 28, 1, &user_area_changed_flag);
	grid_d51_bitmap_write_bit(user_area_buffer, 29, 1, &user_area_changed_flag);
		
		
		
	if (user_area_changed_flag == 1){
			
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Updating User Row");
		_user_area_write(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);
			
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "System Reset");
		NVIC_SystemReset();
			
	}else{
			
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Unchanged User Row");
	}
	
}



uint8_t grid_d51_boundary_scan(uint32_t* result_bitmap){


	result_bitmap[0] = 0;
	result_bitmap[1] = 0;
	result_bitmap[2] = 0;
	result_bitmap[3] = 0;



	// set bit high if there was an error
	uint32_t error_bitmap[4] = {0};
	uint32_t error_count = 0;

	uint8_t PIN_GND = 254;
	uint8_t PIN_VDD = 253;


	uint8_t PIN_RST = 252;
	uint8_t PIN_USB = 251;
	uint8_t PIN_SWD = 250;
	uint8_t PIN_DBG = 249;

	uint8_t PIN_CORE = 248;
	uint8_t PIN_VSW = 247;

	uint8_t SPECIAL_CODES = 240; 



	// START OF PACKAGE PIN DEFINITIONS

	uint8_t pins[100] = {255};

	pins [0]  = GPIO(GPIO_PORTB, 3);

	pins [1]  = GPIO(GPIO_PORTA, 0);
	pins [2]  = GPIO(GPIO_PORTA, 1);
	pins [3]  = GPIO(GPIO_PORTC, 0);
	pins [4]  = GPIO(GPIO_PORTC, 1);
	pins [5]  = GPIO(GPIO_PORTC, 2);
	pins [6]  = GPIO(GPIO_PORTC, 3);
	pins [7]  = GPIO(GPIO_PORTA, 2);
	pins [8]  = GPIO(GPIO_PORTA, 3);
	pins [9]  = GPIO(GPIO_PORTB, 4);
	pins [10] = GPIO(GPIO_PORTB, 5);
	pins [11] = PIN_GND;				//GNDANA
	pins [12] = PIN_VDD;				//VDDANA
	pins [13] = GPIO(GPIO_PORTB, 6);
	pins [14] = GPIO(GPIO_PORTB, 7);
	pins [15] = GPIO(GPIO_PORTB, 8);
	pins [16] = GPIO(GPIO_PORTB, 9);
	pins [17] = GPIO(GPIO_PORTA, 4);
	pins [18] = GPIO(GPIO_PORTA, 5);
	pins [19] = GPIO(GPIO_PORTA, 6);
	pins [20] = GPIO(GPIO_PORTA, 7);
	pins [21] = GPIO(GPIO_PORTC, 5);
	pins [22] = GPIO(GPIO_PORTC, 6);
	pins [23] = GPIO(GPIO_PORTC, 7);
	pins [24] = PIN_GND;				//GND
	pins [25] = PIN_VDD;				//VDDIOB

	pins [26] = GPIO(GPIO_PORTA, 8);
	pins [27] = GPIO(GPIO_PORTA, 9);
	pins [28] = GPIO(GPIO_PORTA, 10);
	pins [29] = GPIO(GPIO_PORTA, 11);
	pins [30] = PIN_VDD;				//VDDIOB
	pins [31] = PIN_GND;				//GND
	pins [32] = GPIO(GPIO_PORTB, 10);
	pins [33] = GPIO(GPIO_PORTB, 11);
	pins [34] = GPIO(GPIO_PORTB, 12);
	pins [35] = GPIO(GPIO_PORTB, 13);
	pins [36] = GPIO(GPIO_PORTB, 14);
	pins [37] = GPIO(GPIO_PORTB, 15);
	pins [38] = PIN_GND;				//GND
	pins [39] = PIN_VDD;				//VDDIO
	pins [40] = GPIO(GPIO_PORTC, 10);
	pins [41] = GPIO(GPIO_PORTC, 11);
	pins [42] = GPIO(GPIO_PORTC, 12);
	pins [43] = GPIO(GPIO_PORTC, 13);
	pins [44] = GPIO(GPIO_PORTC, 14);
	pins [45] = GPIO(GPIO_PORTC, 15);
	pins [46] = GPIO(GPIO_PORTA, 12);
	pins [47] = GPIO(GPIO_PORTA, 13);
	pins [48] = GPIO(GPIO_PORTA, 14);
	pins [49] = GPIO(GPIO_PORTA, 15);
	pins [50] = PIN_GND;				//GND

	pins [51] = PIN_VDD;				//VDDIO
	pins [52] = GPIO(GPIO_PORTA, 16);
	pins [53] = GPIO(GPIO_PORTA, 17);
	pins [54] = GPIO(GPIO_PORTA, 18);
	pins [55] = GPIO(GPIO_PORTA, 19);
	pins [56] = GPIO(GPIO_PORTC, 16);
	pins [57] = GPIO(GPIO_PORTC, 17);
	pins [58] = GPIO(GPIO_PORTC, 18);
	pins [59] = GPIO(GPIO_PORTC, 19);
	pins [60] = GPIO(GPIO_PORTC, 20);
	pins [61] = GPIO(GPIO_PORTC, 21);
	pins [62] = PIN_GND;				//GND
	pins [63] = PIN_VDD;				//VDDIO
	pins [64] = GPIO(GPIO_PORTB, 16);
	pins [65] = GPIO(GPIO_PORTB, 17);
	pins [66] = GPIO(GPIO_PORTB, 18);
	pins [67] = GPIO(GPIO_PORTB, 19);
	pins [68] = GPIO(GPIO_PORTB, 20);
	pins [69] = GPIO(GPIO_PORTB, 21);
	pins [70] = GPIO(GPIO_PORTA, 20);
	pins [71] = GPIO(GPIO_PORTA, 21);
	pins [72] = GPIO(GPIO_PORTA, 22);
	pins [73] = GPIO(GPIO_PORTA, 23);
	pins [74] = GPIO(GPIO_PORTA, 24);
	pins [75] = GPIO(GPIO_PORTA, 25);

	pins [76] = PIN_GND;				//GND
	pins [77] = PIN_VDD;				//VDDIO
	pins [78] = GPIO(GPIO_PORTB, 22);
	pins [79] = GPIO(GPIO_PORTB, 23);
	pins [80] = GPIO(GPIO_PORTB, 24);
	pins [81] = GPIO(GPIO_PORTB, 25);
	pins [82] = GPIO(GPIO_PORTC, 24);
	pins [83] = GPIO(GPIO_PORTC, 25);
	pins [84] = GPIO(GPIO_PORTC, 26);
	pins [85] = GPIO(GPIO_PORTC, 27);
	pins [86] = GPIO(GPIO_PORTC, 28);
	pins [87] = GPIO(GPIO_PORTA, 27);
	pins [88] = PIN_RST;				//RESETN
	pins [89] = PIN_CORE;				//VDDCORE
	pins [90] = PIN_GND;				//GND
	pins [91] = PIN_VSW;				//VSW
	pins [92] = PIN_VDD;				//VDDIO
	pins [93] = GPIO(GPIO_PORTA, 30);
	pins [94] = GPIO(GPIO_PORTA, 31);
	pins [95] = GPIO(GPIO_PORTB, 30);
	pins [96] = GPIO(GPIO_PORTB, 31);
	pins [97] = GPIO(GPIO_PORTB, 0);
	pins [98] = GPIO(GPIO_PORTB, 1);
	pins [99] = GPIO(GPIO_PORTB, 2);


	// END OF PACKAGE PIN DEFINITIONS
	// START OF GRID COMMON PIN DEFINITIONS


// HWCFG
// FLASH
// SWD
// USB
// MAPMODE


	// END OF GRID COMMON PIN DEFINITIONS
	// START OF GRID MODULE SPECIFIC PIN DEFINITIONS



	// SET PINS SO HWCFG CAN BE READ
	// gpio_set_pin_level(HWCFG_SHIFT, false);
	// gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OUT);
	// gpio_set_pin_function(HWCFG_SHIFT, GPIO_PIN_FUNCTION_OFF);

	// gpio_set_pin_level(HWCFG_CLOCK, false);
	// gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OUT);
	// gpio_set_pin_function(HWCFG_CLOCK, GPIO_PIN_FUNCTION_OFF);

	// gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_IN);
	// gpio_set_pin_pull_mode(HWCFG_DATA, GPIO_PULL_OFF);
	// gpio_set_pin_function(HWCFG_DATA, GPIO_PIN_FUNCTION_OFF);

	// // READ THE HWCFG REGISTER
	// grid_sys_get_hwcfg();

	// // RESET HWCFG PINS TO THEIR DEFAULT STATE	

	// gpio_set_pin_level(HWCFG_SHIFT, false);
	// gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OFF);
	// gpio_set_pin_function(HWCFG_SHIFT, GPIO_PIN_FUNCTION_OFF);

	// gpio_set_pin_level(HWCFG_CLOCK, false);
	// gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OFF);
	// gpio_set_pin_function(HWCFG_CLOCK, GPIO_PIN_FUNCTION_OFF);

	// gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_OFF);
	// gpio_set_pin_pull_mode(HWCFG_DATA, GPIO_PULL_OFF);
	// gpio_set_pin_function(HWCFG_DATA, GPIO_PIN_FUNCTION_OFF);	


	// END OF GRID MODULE SPECIFIC PIN DEFINITIONS


	// TEST ALL OF THE GPIOs 
	for (uint8_t i=0; i<100; i++){

		uint8_t pin_failed = false;

		uint8_t pin_this = pins[i];
		uint8_t pin_prev = pins[(i-1+100)%100];
		uint8_t pin_next = pins[(i+1)%100];


		//pin is real gpio pin
		if (pin_this < SPECIAL_CODES){

			if (!pin_failed){
				// SET TO INPUT, PULLDOWN
				gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
				gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);

				// TEST IF pins[i] reads low
				if (gpio_get_pin_level(pin_this) != false){
					pin_failed = true;
				}

				gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
				gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
			}

			if (!pin_failed){
				// SET TO INPUT, PULLUP
				gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
				gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);

				// TEST IF pins[i] reads high
				if (gpio_get_pin_level(pin_this) != true){
					pin_failed = true;
				}
								
				gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
				gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
			}

			// TEST PREV 
			if (!pin_failed){
				
				if (pin_prev < SPECIAL_CODES){

					// SET THIS PIN AS INPUT, PULLDOWN
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);	

					// SET PREVIOUS AS OUTPUT, HIGH
					gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OUT);
					gpio_set_pin_level(pin_prev, true);	
					// READ THISPIN				
					if (gpio_get_pin_level(pin_this) != false){
						pin_failed = true;
					}

					// SET THIS PIN AS INPUT, PULLUP					
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);	
					// SET PREVIOUS AS OUTPUT, LOW
					gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OUT);
					gpio_set_pin_level(pin_prev, false);						
					// READ THISPIN
					if (gpio_get_pin_level(pin_this) != true){
						pin_failed = true;
					}

					// RESET PREVIOUS TO DEFAULT STATE
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
					gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OFF);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
					gpio_set_pin_level(pin_prev, false);			
				}
			}

			// TEST NEXT
			if (!pin_failed){
				
				if (pin_next < SPECIAL_CODES){

					// SET THIS PIN AS INPUT, PULLDOWN
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);	

					// SET NEXT AS OUTPUT, HIGH
					gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OUT);
					gpio_set_pin_level(pin_next, true);	
					// READ THISPIN				
					if (gpio_get_pin_level(pin_this) != false){
						pin_failed = true;
					}

					// SET THIS PIN AS INPUT, PULLUP					
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);	
					// SET NEXT AS OUTPUT, LOW
					gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OUT);
					gpio_set_pin_level(pin_next, false);						
					// READ THISPIN
					if (gpio_get_pin_level(pin_this) != true){
						pin_failed = true;
					}

					// RESET NEXT TO DEFAULT STATE
					gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
					gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OFF);
					gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
					gpio_set_pin_level(pin_next, false);		
				}
			}

			//RESET THIS PIN TO DEFAULT STATE
		
			if (pin_failed){
				//Sorry, pin failed

				uint8_t error_array_index = (i-1)/25;
				uint8_t error_bit_index = (i-1)%25;

				error_count++;
				error_bitmap[error_array_index] |= (1<<error_bit_index);

				result_bitmap[0] = error_bitmap[0];
				result_bitmap[1] = error_bitmap[1];
				result_bitmap[2] = error_bitmap[2];
				result_bitmap[3] = error_bitmap[3];

			}

		}

	}


}



uint32_t grid_d51_dwt_enable(){

    if (GRID_D51_DWT_CTRL != 0) {                  // See if DWT is available
        GRID_D51_DEMCR      |= 1 << 24;            // Set bit 24
        GRID_D51_DWT_CYCCNT  = 0;                
        GRID_D51_DWT_CTRL   |= 1 << 0;             // Set bit 0

        printf("Debug Watch and Trace enabled!\r\n");
    }
    else{
        printf("Debug Watch and Trace not supported!\r\n");
    }
}



uint32_t grid_d51_dwt_cycles_read(){

	return GRID_D51_DWT_CYCCNT;

}




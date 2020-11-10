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
	
	
	#ifdef HARDWARETEST
	
	#include "grid/grid_hardwaretest.h"
	
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
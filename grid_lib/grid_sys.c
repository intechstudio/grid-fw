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
	
	// Read the register for the first time, then later just return the saved value

	if (grid_sys_hwfcg == -1){

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
		
		grid_sys_hwfcg = hwcfg_value;
		
	}

	
	return grid_sys_hwfcg;

}




#define GRID_SYS_NORTH	0
#define GRID_SYS_EAST	1
#define GRID_SYS_SOUTH	2
#define GRID_SYS_WEST	3

static uint8_t grid_sys_pingmessage[2] = "P";

volatile uint8_t grid_sys_ping_counter[4] = {0, 0, 0, 0};

volatile uint32_t grid_sys_rx_counter[4] = {0, 0, 0, 0};
volatile uint32_t grid_sys_tx_counter[4] = {0, 0, 0, 0};




static void tx_cb_USART_GRID(const struct usart_async_descriptor *const descr)
{
	/* Transfer completed */
		
	if (descr == &USART_EAST){
		grid_sys_tx_counter[GRID_SYS_EAST]++;
	}
	if (descr == &USART_WEST){
		grid_sys_tx_counter[GRID_SYS_WEST]++;
	}
	
}

static void rx_cb_USART_GRID(const struct usart_async_descriptor *const descr)
{
	/* Transfer completed */
	
	if (descr == &USART_EAST){
		grid_sys_rx_counter[GRID_SYS_EAST]++;
	}
	if (descr == &USART_WEST){
		grid_sys_rx_counter[GRID_SYS_WEST]++;
	}
	
}

void grid_sys_uart_init(){

	usart_async_register_callback(&USART_EAST, USART_ASYNC_TXC_CB, tx_cb_USART_GRID);
	usart_async_register_callback(&USART_EAST, USART_ASYNC_RXC_CB, rx_cb_USART_GRID);	
	
	usart_async_get_io_descriptor(&USART_EAST, &grid_sys_east_io);
	
	usart_async_enable(&USART_EAST);
	
	
	usart_async_register_callback(&USART_WEST, USART_ASYNC_TXC_CB, tx_cb_USART_GRID);
	usart_async_register_callback(&USART_WEST, USART_ASYNC_RXC_CB, rx_cb_USART_GRID);
	
	usart_async_get_io_descriptor(&USART_WEST, &grid_sys_west_io);
	
	usart_async_enable(&USART_WEST);

}

	
void grid_msg_process_all(){
	
	grid_msg_process(&USART_EAST, GRID_SYS_EAST);
	grid_msg_process(&USART_WEST, GRID_SYS_WEST);
		
}	

void grid_msg_process(const struct usart_async_descriptor *const descr, uint8_t offset){
	
	uint8_t character;
	
	
	if (usart_async_is_rx_not_empty(descr)){
		
		while(io_read(&(*descr).io, &character, 1) == 1){
			
			if (character == GRID_MSG_PING){
				
				grid_sys_ping_counter[offset] += 1;
				
				
			}
			
		}	
		
	}
	

	
	
}

void grid_sys_ping_all(){
	
	grid_sys_ping(&USART_EAST);
	grid_sys_ping(&USART_WEST);

}

void grid_sys_ping(const struct usart_async_descriptor *const descr){
	
		
	io_write(&(*descr).io, grid_sys_pingmessage ,1);
	
}


	
	
	
	
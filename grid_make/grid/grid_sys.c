/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_sys.h"

int32_t grid_utility_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum grid_task grid_task_enter_task(struct grid_task_model* mod, enum grid_task next_task){
	
	
	enum grid_task previous_task = mod->current_task;
	mod->current_task = next_task;
	return previous_task;
	
}

void grid_task_leave_task(struct grid_task_model* mod, enum grid_task previous_task){
	
	mod->current_task = previous_task;
	
}

void grid_task_timer_tick(struct grid_task_model* mod){
	
	mod->timer[mod->current_task]++;
	
}

void grid_task_timer_reset(struct grid_task_model* mod){
	
	for (uint8_t i=0; i<GRID_TASK_NUMBER; i++){
		mod->timer[i] = 0;
	}
	
}

uint32_t grid_task_timer_read(struct grid_task_model* mod, enum grid_task task){

	return 	mod->timer[task];
	
}


void grid_debug_print_text(uint8_t* debug_string){
	
	uint32_t debug_string_length = strlen(debug_string);
	
	struct grid_msg message;
	
	grid_msg_init_header(&message, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

	grid_msg_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_start);
	grid_msg_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_body_append_printf(&message, debug_string);
	grid_msg_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&message);
	grid_msg_packet_send_everywhere(&message);
		
}

void grid_debug_printf(char const *fmt, ...){

	va_list ap;


	uint8_t temp[100] = {0};

	va_start(ap, fmt);

	vsnprintf(temp, 99, fmt, ap);

	va_end(ap);

	grid_debug_print_text(temp);

	return;
}
//====================== grid sys unittest ===================================//

uint32_t grid_sys_unittest(void){


	if(grid_unittest_group_init(&grid_unittest_state, "grid_sys::checksum"))
	{
		if(grid_unittest_case_init(&grid_unittest_state, "Checksum Read/Calculate"))
		{ // Read/Calculate
			
			uint8_t length = 8;
			uint8_t packet[8] = {1, 0, 2, 2, 4, 0, 0, 10};
		
			uint8_t checksum_calc = grid_msg_calculate_checksum_of_packet_string(packet, length);
			uint8_t checksum_read = grid_msg_checksum_read(packet, length);
				
			char str[100] = {0};
			sprintf(str, "packet{%d, %d, %d, %d, %d, %d, %d, %d} Read: %d, Calculate: %d", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7], checksum_read, checksum_calc);	
				
			if (checksum_calc != checksum_read){		
				grid_unittest_case_pass(&grid_unittest_state,str);
			}
			else{
				grid_unittest_case_fail(&grid_unittest_state,str);
			}
			
		}
		if (grid_unittest_case_init(&grid_unittest_state, "Checksum Write/Calculate")) // Write/Calculate	
		{	
		
		
			uint8_t length = 8;
			uint8_t packet[8] = {1, 0, 2, 2, 4, 0, 0, 10};
					
			uint8_t checksum_calc = grid_msg_calculate_checksum_of_packet_string(packet, length);
		
			grid_msg_checksum_write(packet, length, checksum_calc);
		
			uint8_t checksum_read = grid_msg_checksum_read(packet, length);
				
			char str[100] = {0};
			sprintf(str, "packet{%d, %d, %d, %d, %d, %d, %d, %d} Read: %d, Calculate: %d", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7], checksum_read, checksum_calc);	
		
		
			if (checksum_calc == checksum_read){		
				grid_unittest_case_pass(&grid_unittest_state,str);
			}
			else{
				grid_unittest_case_fail(&grid_unittest_state,str);
			}
				
		}	
		
		
	}
	
	grid_unittest_case_init(&grid_unittest_state, "Checksum Overwrite");
	grid_unittest_case_fail(&grid_unittest_state, "Parapaprikas");
		
	grid_unittest_group_done(&grid_unittest_state);
		

	//grid_unittest_case_init(&grid_unittest_state, );

	return 1;
}








//=============================== USART TX COMPLETE ==============================//

static void tx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_N);
}

static void tx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_E);
}

static void tx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_S);
}

static void tx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_W);
}

void tx_cb_USART_GRID(struct grid_port* const por){
	
	
	for(uint32_t i=0; i<por->tx_double_buffer_status; i++){
		por->tx_double_buffer[i] = 0;
	}
	por->tx_double_buffer_status = 0;	
}


static void rx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_N);
}

static void rx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_E);
}

static void rx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_S);
}

static void rx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_W);
}

void rx_cb_USART_GRID(struct grid_port* const por){
	
}

volatile dmatest = 0;

void dma_transfer_complete_n_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_N);
}
void dma_transfer_complete_e_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_E);
}
void dma_transfer_complete_s_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_S);
}
void dma_transfer_complete_w_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_W);
}
void dma_transfer_complete(struct grid_port* por){

	grid_sys_port_reset_dma(por);

}





static void err_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_N);
}

static void err_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_E);
}

static void err_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_S);
}

static void err_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	
	err_cb_USART_GRID(&GRID_PORT_W);
}


void err_cb_USART_GRID(struct grid_port* const por){

	por->usart_error_flag = 1;	
	//uint8_t character = (((Sercom *)((*por->usart).device.hw))->USART.DATA.reg);

	//printf("@%d\r\n", character);

	//usart_async_disable(por->usart);
}



//====================== DMA CONFIGURATION FOR GRID USART RX C ===================================//

#define DMA_NORTH_RX_CHANNEL	0
#define DMA_EAST_RX_CHANNEL		1
#define DMA_SOUTH_RX_CHANNEL	2
#define DMA_WEST_RX_CHANNEL		3

void grid_sys_port_reset_dma(struct grid_port* por){
	
	hri_dmac_clear_CHCTRLA_ENABLE_bit(DMAC, por->dma_channel);
	_dma_enable_transaction(por->dma_channel, false);

}


void grid_sys_uart_init(){
	
	
	// RX PULLUP 
	gpio_set_pin_pull_mode(PC28, GPIO_PULL_UP);
	gpio_set_pin_pull_mode(PC16, GPIO_PULL_UP);
	gpio_set_pin_pull_mode(PC12, GPIO_PULL_UP);
	gpio_set_pin_pull_mode(PB09, GPIO_PULL_UP);
	
	usart_async_register_callback(&USART_NORTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_N);
	usart_async_register_callback(&USART_EAST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_E);
	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_S);
	usart_async_register_callback(&USART_WEST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_W);
			
	// Set parity for grid uart communication
	// usart_async_set_parity(&USART_NORTH, USART_PARITY_ODD);
	// usart_async_set_parity(&USART_EAST, USART_PARITY_ODD);	
	// usart_async_set_parity(&USART_SOUTH, USART_PARITY_ODD);
	// usart_async_set_parity(&USART_WEST, USART_PARITY_ODD);

	// _usart_async_set_stop_bits(&USART_NORTH, USART_STOP_BITS_ONE);
	// _usart_async_set_stop_bits(&USART_EAST, USART_STOP_BITS_TWO);	
	// _usart_async_set_stop_bits(&USART_SOUTH, USART_STOP_BITS_TWO);
	// _usart_async_set_stop_bits(&USART_WEST, USART_STOP_BITS_TWO);
	
	// Set callback function for parity error
	// usart_async_register_callback(&USART_NORTH, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_N);
	// usart_async_register_callback(&USART_EAST, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_E);
	// usart_async_register_callback(&USART_SOUTH, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_S);
	// usart_async_register_callback(&USART_WEST, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_W);
	
// 	usart_async_register_callback(&USART_NORTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_N);
// 	usart_async_register_callback(&USART_EAST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_E);
// 	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_S);
// 	usart_async_register_callback(&USART_WEST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_W);

	
	usart_async_get_io_descriptor(&USART_NORTH, &grid_sys_north_io);
	usart_async_get_io_descriptor(&USART_EAST,  &grid_sys_east_io);
	usart_async_get_io_descriptor(&USART_SOUTH, &grid_sys_south_io);
	usart_async_get_io_descriptor(&USART_WEST,  &grid_sys_west_io);


	usart_async_enable(&USART_NORTH);
	usart_async_enable(&USART_EAST);
	usart_async_enable(&USART_SOUTH);
	usart_async_enable(&USART_WEST);
	

}



void grid_sys_dma_rx_init_one(struct grid_port* por, uint32_t buffer_length, void* transfer_done_cb() ){
	
	
	uint8_t dma_rx_channel = por->dma_channel;
	
	_dma_set_source_address(dma_rx_channel, (uint32_t) & (((Sercom *)((*por->usart).device.hw))->USART.DATA.reg));
	_dma_set_destination_address(dma_rx_channel, (uint32_t *)por->rx_double_buffer);
	_dma_set_data_amount(dma_rx_channel, (uint32_t)buffer_length);
	
	struct _dma_resource *resource_rx;
	_dma_get_channel_resource(&resource_rx, dma_rx_channel);
	
	resource_rx->dma_cb.transfer_done = transfer_done_cb;
	_dma_set_irq_state(dma_rx_channel, DMA_TRANSFER_COMPLETE_CB, true);
	
	//resource_rx->dma_cb.error         = function_cb;
	_dma_enable_transaction(dma_rx_channel, false);
	

}

void grid_sys_dma_rx_init(){
	
	grid_sys_dma_rx_init_one(&GRID_PORT_N, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_n_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_E, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_e_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_S, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_s_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_W, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_w_cb);

	NVIC_SetPriority(DMAC_0_IRQn, 0);
	NVIC_SetPriority(DMAC_1_IRQn, 0);
	NVIC_SetPriority(DMAC_2_IRQn, 0);
	NVIC_SetPriority(DMAC_3_IRQn, 0);
	
}

void grid_sys_init(struct grid_sys_model* mod){
	
	mod->uptime = 0;
	mod->reset_cause = hri_rstc_read_RCAUSE_reg(RSTC);
	
	mod->midirx_any_enabled = 1;
	mod->midirx_sync_enabled = 0;

	mod->hwfcg = -1;
	mod->heartbeat_type = 0;

	mod->module_x = 0; // 0 because this is signed int
	mod->module_y = 0; // 0 because this is signed int
	mod->module_rot = GRID_SYS_DEFAULT_ROTATION;

	mod->lastheader_config.status = -1;
	mod->lastheader_pagestore.status = -1;
	mod->lastheader_nvmerase.status = -1;
	mod->lastheader_pagediscard.status = -1;
	mod->lastheader_pageclear.status = -1;

    
	rand_sync_enable(&RAND_0);	
	mod->sessionid = rand_sync_read8(&RAND_0);
    
	
	mod->bank_color_r[0] = 0;
	mod->bank_color_g[0] = 100;
	mod->bank_color_b[0] = 200;
	
	mod->bank_color_r[1] = 200;
	mod->bank_color_g[1] = 100;
	mod->bank_color_b[1] = 0;
		
	mod->bank_color_r[2] = 50;
	mod->bank_color_g[2] = 200;
	mod->bank_color_b[2] = 50;
	
	mod->bank_color_r[3] = 100;
	mod->bank_color_g[3] = 0;
	mod->bank_color_b[3] = 200;
	
	mod->bank_enabled[0] = 1;
	mod->bank_enabled[1] = 1;
	mod->bank_enabled[2] = 1;
	mod->bank_enabled[3] = 1;
	
	mod->bank_activebank_color_r = 0;
	mod->bank_activebank_color_g = 0;
	mod->bank_activebank_color_b = 0;
	
	mod->mapmodestate = 1;
	
	mod->bank_active_changed = 0;
	mod->bank_setting_changed_flag = 0;
	
	mod->bank_init_flag = 0;


	mod->bank_activebank_number = 0;


	mod->editor_connected = 0;
	mod->editor_heartbeat_lastrealtime = 0;

}


uint8_t grid_sys_bank_enable(struct grid_sys_model* mod, uint8_t banknumber){
	
	if (banknumber<GRID_SYS_BANK_MAXNUMBER){
		mod->bank_enabled[banknumber] = 1;
	}
	

}

uint8_t grid_sys_bank_disable(struct grid_sys_model* mod, uint8_t banknumber){
	
	if (banknumber<GRID_SYS_BANK_MAXNUMBER){
		mod->bank_enabled[banknumber] = 0;
	}
	
	
	
}

uint8_t grid_sys_bank_set_color(struct grid_sys_model* mod, uint8_t banknumber, uint32_t rgb){
	
	if (banknumber>GRID_SYS_BANK_MAXNUMBER){
		return false;
	}
	
	// 0x00RRGGBB
	
	mod->bank_color_r[banknumber] = ((rgb&0x00FF0000)>>16);
	mod->bank_color_g[banknumber] = ((rgb&0x0000FF00)>>8);
	mod->bank_color_b[banknumber] = ((rgb&0x000000FF)>>0);
	
}

uint32_t grid_sys_bank_get_color(struct grid_sys_model* mod, uint8_t banknumber){
	
	if (banknumber>GRID_SYS_BANK_MAXNUMBER){
		return false;
	}
	
		
}


uint8_t grid_sys_get_bank_num(struct grid_sys_model* mod){
	
	return mod->bank_activebank_number;
}

uint8_t grid_sys_get_bank_valid(struct grid_sys_model* mod){
	
	return mod->bank_activebank_valid;
}

uint8_t grid_sys_get_bank_red(struct grid_sys_model* mod){
	
	return mod->bank_activebank_color_r;
}

uint8_t grid_sys_get_bank_gre(struct grid_sys_model* mod){
	
	return mod->bank_activebank_color_g;
}

uint8_t grid_sys_get_bank_blu(struct grid_sys_model* mod){

	return mod->bank_activebank_color_b;
}

uint8_t grid_sys_get_map_state(struct grid_sys_model* mod){

	return mod->mapmodestate;

}


uint8_t grid_sys_get_bank_next(struct grid_sys_model* mod){
		
	uint8_t current_active = grid_sys_get_bank_num(mod);
		
	for (uint8_t i=0; i<GRID_SYS_BANK_MAXNUMBER; i++){
		
		uint8_t bank_check = (current_active+i+1)%GRID_SYS_BANK_MAXNUMBER;
		
		if (mod->bank_enabled[bank_check] == 1){
			
			return bank_check;
		}
		
	}
	
	return current_active;
	
}

uint8_t grid_sys_get_bank_number_of_first_valid(struct grid_sys_model* mod){
	
	uint8_t current_active = grid_sys_get_bank_num(mod);
	
	for (uint8_t i=0; i<GRID_SYS_BANK_MAXNUMBER; i++){
		
		if (mod->bank_enabled[i] == 1){
			
			return i;
		}
		
	}
	
	return 255;
	
}


void grid_sys_set_bank(struct grid_sys_model* mod, uint8_t banknumber){
	
	uint8_t old_page = mod->bank_activebank_number;
	
	if (banknumber == 255){
			
		//mod->bank_activebank_number = 0;
		mod->bank_activebank_valid = 0;
		
		mod->bank_active_changed = 1;
				
		mod->bank_activebank_color_r = 127;
		mod->bank_activebank_color_g = 127;
		mod->bank_activebank_color_b = 127;

		
	}
	else if (banknumber<GRID_SYS_BANK_MAXNUMBER){
							
							
		mod->bank_init_flag = 1;
		
		
		if (mod->bank_enabled[banknumber] == 1){
			
			mod->bank_activebank_number = banknumber;
			mod->bank_activebank_valid = 1;
			
			mod->bank_active_changed = 1;
			
			mod->bank_activebank_color_r = mod->bank_color_r[mod->bank_activebank_number];
			mod->bank_activebank_color_g = mod->bank_color_g[mod->bank_activebank_number];
			mod->bank_activebank_color_b = mod->bank_color_b[mod->bank_activebank_number];	
					
		}
		else{
			
			//grid_debug_print_text("NOT ENABLED");
			
		}

	
	}
	else{
		
		//grid_debug_print_text("Invalid Bank Number");	
				
	}

	uint8_t new_page = mod->bank_activebank_number;

	

	
	


	
}



// REALTIME

uint32_t grid_sys_rtc_get_time(struct grid_sys_model* mod){
	return mod->realtime;
}


void grid_sys_rtc_set_time(struct grid_sys_model* mod, uint32_t tvalue){
	
	mod->realtime = tvalue;
}

uint32_t grid_sys_rtc_get_elapsed_time(struct grid_sys_model* mod, uint32_t t_old){
	
	return mod->realtime-t_old;
	
	

}

void grid_sys_rtc_tick_time(struct grid_sys_model* mod){
	
	mod->realtime++;
	if (mod->uptime != -1){
		mod->uptime++;
	}
	
}

uint8_t grid_sys_read_hex_char_value(uint8_t ascii, uint8_t* error_flag){
		
	uint8_t result = 0;
	
	if (ascii>47 && ascii<58){
		result = ascii-48;
	}
	else if(ascii>96 && ascii<103){
		result = ascii - 97 + 10;
	}
	else{
		// wrong input
		if (error_flag != NULL){
			*error_flag = ascii;
		}
	}
	
	return result;	
}

uint32_t grid_sys_read_hex_string_value(uint8_t* start_location, uint8_t length, uint8_t* error_flag){
	
	uint32_t result  = 0;
	
	for(uint8_t i=0; i<length; i++){
		
		result += grid_sys_read_hex_char_value(start_location[i], error_flag) << (length-i-1)*4;

		
	}

	return result;
}

void grid_sys_write_hex_string_value(uint8_t* start_location, uint8_t size, uint32_t value){
	
	uint8_t str[10];
	
	sprintf(str, "%08x", value);
		
	for(uint8_t i=0; i<size; i++){	
		start_location[i] = str[8-size+i];	
	}

}




uint32_t grid_sys_get_id(uint32_t* return_array){
			
	return_array[0] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_0);
	return_array[1] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_1);
	return_array[2] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_2);
	return_array[3] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_3);
	
	return 1;
	
}

uint32_t grid_sys_get_hwcfg(struct grid_sys_model* mod){
	
	// Read the register for the first time, then later just return the saved value

	if (mod->hwfcg == -1){

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
		
		mod->hwfcg = hwcfg_value;
		
	}

	
	return mod->hwfcg;

}


#define GRID_SYS_NORTH	0
#define GRID_SYS_EAST	1
#define GRID_SYS_SOUTH	2
#define GRID_SYS_WEST	3



void grid_sys_ping_all(){
		
	grid_sys_ping(&GRID_PORT_N);
	grid_sys_ping(&GRID_PORT_E);
	grid_sys_ping(&GRID_PORT_S);
	grid_sys_ping(&GRID_PORT_W);
	
}

uint8_t grid_msg_calculate_checksum_of_packet_string(uint8_t* str, uint32_t length){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<length-3; i++){
		checksum ^= str[i];
	}
	
	return checksum;
	
}

uint8_t grid_msg_calculate_checksum_of_string(uint8_t* str, uint32_t length){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<length; i++){
		checksum ^= str[i];
	}
	
	return checksum;
	
}


uint8_t grid_msg_checksum_read(uint8_t* str, uint32_t length){
	uint8_t error_flag;
	return grid_sys_read_hex_string_value(&str[length-3], 2, &error_flag);
}

void grid_msg_checksum_write(uint8_t* message, uint32_t length, uint8_t checksum){
	
// 	uint8_t checksum_string[4];
// 
// 	sprintf(checksum_string, "%02x", checksum);
// 
// 	message[length-3] = checksum_string[0];
// 	message[length-2] = checksum_string[1];
	
	grid_sys_write_hex_string_value(&message[length-3], 2, checksum);
	
}


// MESSAGE PARAMETER FUNCTIONS

uint32_t grid_msg_get_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint8_t* error){
		
	return grid_sys_read_hex_string_value(&message[offset], length, error);	
}

uint32_t grid_msg_set_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint32_t value, uint8_t* error){
	
	grid_sys_write_hex_string_value(&message[offset], length, value);
	
}




// RECENT MESSAGES

uint8_t grid_msg_find_recent(struct grid_sys_model* model, uint32_t fingerprint){
	
	for(GRID_SYS_RECENT_MESSAGES_INDEX_T i = 0; i<GRID_SYS_RECENT_MESSAGES_LENGTH; i++){
		
		if (model->recent_messages[i%GRID_SYS_RECENT_MESSAGES_LENGTH] == fingerprint){
			
			return 1;
			
		}
		
	}
	
	return 0;
}

void grid_msg_push_recent(struct grid_sys_model* model, uint32_t fingerprint){
	
	model->recent_messages_index+=1;
	model->recent_messages_index%=GRID_SYS_RECENT_MESSAGES_LENGTH;
	
	model->recent_messages[model->recent_messages_index] = fingerprint;
	
}

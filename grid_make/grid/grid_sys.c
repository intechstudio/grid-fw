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


void grid_debug_print_text(uint8_t* debug_string){
	
	uint32_t debug_string_length = strlen(debug_string);
	
	struct grid_msg message;
	
	grid_msg_init_header(&message, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

	grid_msg_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_start);
	grid_msg_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_body_append_printf(&message, debug_string);
	grid_msg_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&message);
	grid_sys_packet_send_everywhere(&message);
		
}


void grid_websocket_print_text(uint8_t* debug_string){
	
	uint32_t debug_string_length = strlen(debug_string);
	
	struct grid_msg message;
	
	grid_msg_init_header(&message, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

	grid_msg_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_start);
	grid_msg_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_body_append_printf(&message, debug_string);
	grid_msg_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_end);

	grid_msg_packet_close(&message);
	grid_sys_packet_send_everywhere(&message);
		
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

uint8_t	grid_sys_packet_send_everywhere(struct grid_msg* msg){
	
	uint32_t message_length = grid_msg_packet_get_length(msg);
	
	if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, message_length)){

		for(uint32_t i = 0; i<message_length; i++){

			grid_buffer_write_character(&GRID_PORT_U.rx_buffer, grid_msg_packet_send_char(msg, i));
		}

		grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);

		return 1;
	}
	else{
		
		return 0;
	}
	
	
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
	
	// PLATFORM SPECIFIC INITIALIZERS

	mod->reset_cause = grid_d51_get_reset_cause();
	grid_msg_state.sessionid = grid_d51_get_random_8();
	mod->hwfcg = grid_d51_get_hwcfg();

	uint32_t uniqueid[4] = {0};
	grid_d51_get_id(uniqueid);			

	mod->uniqueid_array[0] = uniqueid[0];
	mod->uniqueid_array[1] = uniqueid[1];
	mod->uniqueid_array[2] = uniqueid[2];
	mod->uniqueid_array[3] = uniqueid[3];

	// LOCAL INITIALIZERS
	mod->uptime = 0;

	
	mod->midirx_any_enabled = 1;
	mod->midirx_sync_enabled = 0;

	mod->heartbeat_type = 0;

	mod->module_x = 0; // 0 because this is signed int
	mod->module_y = 0; // 0 because this is signed int
	mod->module_rot = GRID_SYS_DEFAULT_ROTATION;

	mod->lastheader_config.status = -1;
	mod->lastheader_pagestore.status = -1;
	mod->lastheader_nvmerase.status = -1;
	mod->lastheader_pagediscard.status = -1;
	mod->lastheader_pageclear.status = -1;

	
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



#define GRID_SYS_NORTH	0
#define GRID_SYS_EAST	1
#define GRID_SYS_SOUTH	2
#define GRID_SYS_WEST	3


uint32_t grid_sys_get_hwcfg(struct grid_sys_model* mod){

	return mod->hwfcg;
}

uint32_t grid_sys_get_id(struct grid_sys_model* mod, uint32_t* return_array){
			
	return_array[0] = mod->uniqueid_array[0];
	return_array[1] = mod->uniqueid_array[1];
	return_array[2] = mod->uniqueid_array[2];
	return_array[3] = mod->uniqueid_array[3];
	
	return 1;
	
}



void grid_sys_ping_all(){
		
	grid_sys_ping(&GRID_PORT_N);
	grid_sys_ping(&GRID_PORT_E);
	grid_sys_ping(&GRID_PORT_S);
	grid_sys_ping(&GRID_PORT_W);
	
}

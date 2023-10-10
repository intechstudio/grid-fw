/*
 * grid_d51_uart.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */ 

#include "grid_d51_uart.h"


static volatile struct grid_port* uart_port_array[4] = {0};


void grid_d51_uart_port_reset_dma(uint8_t dma_channel){
	
	hri_dmac_clear_CHCTRLA_ENABLE_bit(DMAC, dma_channel);
	_dma_enable_transaction(dma_channel, false);

}


//=============================== USART TX COMPLETE ==============================//

static void tx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(uart_port_array[0]);
}

static void tx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(uart_port_array[1]);
}

static void tx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(uart_port_array[2]);
}

static void tx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(uart_port_array[3]);
}

void tx_cb_USART_GRID(struct grid_port* const por){
	
	
	for(uint32_t i=0; i<por->tx_double_buffer_status; i++){
		por->tx_double_buffer[i] = 0;
	}
	por->tx_double_buffer_status = 0;	
}


static void rx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(uart_port_array[0]);
}

static void rx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(uart_port_array[1]);
}

static void rx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(uart_port_array[2]);
}

static void rx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(uart_port_array[3]);
}

void rx_cb_USART_GRID(struct grid_port* const por){
	
}

volatile dmatest = 0;

static void dma_transfer_complete_n_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(uart_port_array[0]);
}
static void dma_transfer_complete_e_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(uart_port_array[1]);
}
static void dma_transfer_complete_s_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(uart_port_array[2]);
}
static void dma_transfer_complete_w_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(uart_port_array[3]);
}
static void dma_transfer_complete(struct grid_port* por){

	uint8_t direction = por->direction;

	if (direction == GRID_CONST_NORTH){
		grid_d51_uart_port_reset_dma(DMA_NORTH_RX_CHANNEL);
	}
	else if (direction == GRID_CONST_EAST){
		grid_d51_uart_port_reset_dma(DMA_EAST_RX_CHANNEL);
	}
	else if (direction == GRID_CONST_SOUTH){
		grid_d51_uart_port_reset_dma(DMA_SOUTH_RX_CHANNEL);
	}
	else if (direction == GRID_CONST_WEST){
		grid_d51_uart_port_reset_dma(DMA_WEST_RX_CHANNEL);
	}
	else{

	}

}





static void err_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(uart_port_array[0]);
}

static void err_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(uart_port_array[1]);
}

static void err_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(uart_port_array[2]);
}

static void err_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	
	err_cb_USART_GRID(uart_port_array[3]);
}


static void err_cb_USART_GRID(struct grid_port* const por){

	por->usart_error_flag = 1;	
	//uint8_t character = (((Sercom *)((*por->usart).device.hw))->USART.DATA.reg);

	//printf("@%d\r\n", character);

	//usart_async_disable(por->usart);
}



void grid_d51_uart_init(){
	
	uart_port_array[0] = grid_transport_get_port(&grid_transport_state, 0);
    uart_port_array[1] = grid_transport_get_port(&grid_transport_state, 1);
    uart_port_array[2] = grid_transport_get_port(&grid_transport_state, 2);
    uart_port_array[3] = grid_transport_get_port(&grid_transport_state, 3);
	
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
	

	grid_d51_uart_dma_rx_init();

}



void grid_d51_uart_dma_rx_init_one(struct usart_async_descriptor* usart, uint8_t channel, uint8_t* buffer, uint32_t length, void* transfer_done_cb() ){
	
	
	uint8_t dma_rx_channel = channel;
	
	_dma_set_source_address(dma_rx_channel, (uint32_t) & (((Sercom *)((*usart).device.hw))->USART.DATA.reg));
	_dma_set_destination_address(dma_rx_channel, (uint32_t *)buffer);
	_dma_set_data_amount(dma_rx_channel, (uint32_t)length);
	
	struct _dma_resource *resource_rx;
	_dma_get_channel_resource(&resource_rx, dma_rx_channel);
	
	resource_rx->dma_cb.transfer_done = transfer_done_cb;
	_dma_set_irq_state(dma_rx_channel, DMA_TRANSFER_COMPLETE_CB, true);
	
	_dma_enable_transaction(dma_rx_channel, false);
	

}

void grid_d51_uart_dma_rx_init(){
	
	grid_d51_uart_dma_rx_init_one(&USART_NORTH, DMA_NORTH_RX_CHANNEL, uart_port_array[0]->rx_double_buffer, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_n_cb);
	grid_d51_uart_dma_rx_init_one(&USART_EAST,  DMA_EAST_RX_CHANNEL, uart_port_array[1]->rx_double_buffer, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_e_cb);
	grid_d51_uart_dma_rx_init_one(&USART_SOUTH, DMA_SOUTH_RX_CHANNEL, uart_port_array[2]->rx_double_buffer, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_s_cb);
	grid_d51_uart_dma_rx_init_one(&USART_WEST,  DMA_WEST_RX_CHANNEL, uart_port_array[3]->rx_double_buffer, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_w_cb);

	NVIC_SetPriority(DMAC_0_IRQn, 0);
	NVIC_SetPriority(DMAC_1_IRQn, 0);
	NVIC_SetPriority(DMAC_2_IRQn, 0);
	NVIC_SetPriority(DMAC_3_IRQn, 0);
	
}

#ifdef GRID_MODULE_P16

#include "../../grid_lib/grid_sys.h"



//====================== USART GRID INIT ===================================//

void grid_sys_uart_init(){

 	usart_async_register_callback(&USART_NORTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_N);
 	usart_async_register_callback(&USART_EAST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_E);
 	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_S);
 	usart_async_register_callback(&USART_WEST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_W);
// 
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



//====================== DMA CONFIGURATION FOR GRID USART RX C ===================================//

#define DMA_NORTH_RX_CHANNEL	0
#define DMA_EAST_RX_CHANNEL		1
#define DMA_SOUTH_RX_CHANNEL	2
#define DMA_WEST_RX_CHANNEL		3

//====================== USART -> DMA -> EVENT -> TIMER -> MAGIC ===================================//
	
	

static uint8_t string[20];
static uint8_t string2[20];
	
// RX TIMEOUT CALLBACKS

static void TIMER_NORTH_RX_TIMEOUT_cb(const struct timer_task *const timer_task)
{
	TIMER_RX_TIMEOUT_cb(&GRID_PORT_N, &TIMER_0, DMA_NORTH_RX_CHANNEL);
}

static void TIMER_EAST_RX_TIMEOUT_cb(const struct timer_task *const timer_task)
{

		
	TIMER_RX_TIMEOUT_cb(&GRID_PORT_E, &TIMER_1, DMA_EAST_RX_CHANNEL);
}
	
static void TIMER_SOUTH_RX_TIMEOUT_cb(const struct timer_task *const timer_task)
{
	TIMER_RX_TIMEOUT_cb(&GRID_PORT_S, &TIMER_2, DMA_SOUTH_RX_CHANNEL);
}
	
static void TIMER_WEST_RX_TIMEOUT_cb(const struct timer_task *const timer_task)
{
	TIMER_RX_TIMEOUT_cb(&GRID_PORT_W, &TIMER_3, DMA_WEST_RX_CHANNEL);
}

void TIMER_RX_TIMEOUT_cb(GRID_PORT_t* por, struct timer_descriptor* timer, uint8_t dma_channel)
{
	//process rx data
		
	volatile uint8_t temp[100];
	volatile uint8_t endcommand = 0;
		
		
	for(uint8_t i = 0; i<100; i++){
			
		temp[i] = por->rx_double_buffer[i];
			
		por->rx_double_buffer[i] = 0;
			
		if (temp[i] == '\n'){
			endcommand = i;
		}
			
			
	}
	
	
	uint8_t str[] = "k\n";
	
	
	// IMPLEMENT CHECKSUM VALIDATOR HERE
	if (endcommand>4){
		str[0] = GRID_MSG_ACKNOWLEDGE;
	}
	else{
		str[0] = GRID_MSG_NACKNOWLEDGE;
	}
	
	if(grid_buffer_write_init(&por->tx_buffer, 2)){
		
		for (uint8_t i=0; i<2; i++)
		{
			grid_buffer_write_character(&por->tx_buffer, str[i]);
		}
		
		grid_buffer_write_acknowledge(&por->tx_buffer);
		
	}

	timer_stop(timer);	
		
	hri_dmac_clear_CHCTRLA_ENABLE_bit(DMAC, dma_channel);
	_dma_enable_transaction(dma_channel, false);
		
	timer_start(timer);		
}

// RX DMA CALLBACKS

void DMA_NORTH_RX_cb(struct _dma_resource *resource)
{
	DMA_RX_cb(DMA_NORTH_RX_CHANNEL, &GRID_PORT_N);
}

void DMA_EAST_RX_cb(struct _dma_resource *resource)
{
	DMA_RX_cb(DMA_EAST_RX_CHANNEL, &GRID_PORT_E);
}

void DMA_SOUTH_RX_cb(struct _dma_resource *resource)
{
	DMA_RX_cb(DMA_SOUTH_RX_CHANNEL, &GRID_PORT_S);
}

void DMA_WEST_RX_cb(struct _dma_resource *resource)
{
	DMA_RX_cb(DMA_WEST_RX_CHANNEL, &GRID_PORT_W);
}

void DMA_RX_cb(uint8_t dma_channel, GRID_PORT_t* por)
{
	
	string2[0]++;
	por->rx_double_buffer[0];
	
	
	if (string2[0] !=0){
		
		while(1){
	
		}
		
	}

}
	
static struct timer_task TIMER_NORTH_RX_TIMEOUT;	
static struct timer_task TIMER_EAST_RX_TIMEOUT;
static struct timer_task TIMER_SOUTH_RX_TIMEOUT;
static struct timer_task TIMER_WEST_RX_TIMEOUT;


void grid_rx_timout_init_one(struct timer_descriptor* timer, struct timer_task* task, void (*function_cb)()){
	
	task->interval = 10;
	task->cb       = TIMER_EAST_RX_TIMEOUT_cb; //DEWBUFGASDASD
	task->mode     = TIMER_TASK_REPEAT;

	timer_add_task(timer, task);
	timer_start(timer);

	
}

void grid_rx_timout_init(){
			
//	grid_rx_timout_init_one(&TIMER_0, &TIMER_NORTH_RX_TIMEOUT, TIMER_NORTH_RX_TIMEOUT_cb);
	grid_rx_timout_init_one(&TIMER_1,  &TIMER_EAST_RX_TIMEOUT,  TIMER_EAST_RX_TIMEOUT_cb);
//	grid_rx_timout_init_one(&TIMER_2, &TIMER_SOUTH_RX_TIMEOUT, TIMER_SOUTH_RX_TIMEOUT_cb);
//	grid_rx_timout_init_one(&TIMER_3,  &TIMER_WEST_RX_TIMEOUT,  TIMER_WEST_RX_TIMEOUT_cb);
				
}

void grid_rx_dma_init_one(uint8_t dma_rx_channel, GRID_PORT_t* por, uint32_t buffer_length, void (*function_cb)()){
	
	_dma_set_source_address(dma_rx_channel, (uint32_t) & (((Sercom *)((*por->usart).device.hw))->USART.DATA.reg));
	_dma_set_destination_address(dma_rx_channel, (uint32_t *)por->rx_double_buffer);
	_dma_set_data_amount(dma_rx_channel, (uint32_t)buffer_length);
	
	struct _dma_resource *resource_rx;
	_dma_get_channel_resource(&resource_rx, dma_rx_channel);
	resource_rx->dma_cb.transfer_done = function_cb;
	resource_rx->dma_cb.error         = function_cb;
	
	_dma_set_irq_state(dma_rx_channel, DMA_TRANSFER_COMPLETE_CB, true);
	_dma_enable_transaction(dma_rx_channel, false);
	
}

void grid_rx_dma_init(){
			
	grid_rx_dma_init_one(DMA_NORTH_RX_CHANNEL, &GRID_PORT_N, 15, DMA_NORTH_RX_cb);
	grid_rx_dma_init_one(DMA_EAST_RX_CHANNEL,  &GRID_PORT_E, 15, DMA_EAST_RX_cb);
	grid_rx_dma_init_one(DMA_SOUTH_RX_CHANNEL, &GRID_PORT_S, 15, DMA_SOUTH_RX_cb);
	grid_rx_dma_init_one(DMA_WEST_RX_CHANNEL,  &GRID_PORT_W, 15, DMA_WEST_RX_cb);
		
}



const uint8_t grid_module_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

		
		
uint8_t		  grid_module_mux = 0;

const uint8_t grid_module_led_buffer_size = 16; // number of ws2812 leds
	
const uint8_t grid_module_ain_buffer_size = 16; // number of analog inputs
const uint8_t grid_module_din_buffer_size = 0; // number of digital inputs
	
	
	
	
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
		
		
		
struct io_descriptor *io;
	
struct io_descriptor *io2;
	
// Define all of the peripheral interrupt callbacks
	
volatile static uint32_t dma_spi_done = 0;

volatile static uint32_t transfer_ready = 1;


volatile static uint8_t ADC_0_conversion_ready = 0;
volatile static uint8_t ADC_1_conversion_ready = 0;

static void convert_cb_ADC_0(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ADC_0_conversion_ready = 1;
}

static void convert_cb_ADC_1(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ADC_1_conversion_ready = 1;
		
		
	/* Make sure both results are ready */
		
	while(ADC_0_conversion_ready==0){}
	while(ADC_1_conversion_ready==0){}
		
	/* Read conversion results */
		
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
		
	uint8_t adc_index_0 = grid_module_mux_lookup[grid_module_mux+8];
	uint8_t adc_index_1 = grid_module_mux_lookup[grid_module_mux+0];
		
	adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
	adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
		
					
	if (grid_adc_get_config(adc_index_0, GRID_ADC_CFG_REVERSED)){
		adcresult_0 = 65535 - adcresult_0;
	}		
		
	if (grid_adc_get_config(adc_index_1, GRID_ADC_CFG_REVERSED)){
		adcresult_1 = 65535 - adcresult_1;
	}	
		
	if (grid_adc_get_config(adc_index_0, GRID_ADC_CFG_BINARY)){
		adcresult_0 = (adcresult_0>10000)*65535;
	}
		
	if (grid_adc_get_config(adc_index_1, GRID_ADC_CFG_BINARY)){
		adcresult_1 = (adcresult_1>10000)*65535;
	}
		
		
		
	grid_ain_add_sample(adc_index_0, adcresult_0);
	grid_ain_add_sample(adc_index_1, adcresult_1);
		
		
	/* Update the multiplexer */
		
	grid_module_mux++;
	grid_module_mux%=8;
		
		
		
	gpio_set_pin_level(MUX_A, grid_module_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_mux/4%2);
		
	/* Start conversion new conversion*/
	ADC_0_conversion_ready = 0;	
	ADC_1_conversion_ready = 0;
		
	adc_async_start_conversion(&ADC_0);			
	adc_async_start_conversion(&ADC_1);
		
}


// DMA SPI CALLBACK
static void tx_complete_cb_GRID_LED(struct _dma_resource *resource)
{
	dma_spi_done = 1;
}





	
/* ============================== GRID_MODULE_INIT() ================================ */
void grid_module_init(void){
		
					

	grid_port_init_all();	
		
		
						
	// Allocate memory for 4 analog input with the filter depth of 3 samples, 14 bit format, 10bit result resolution
	grid_ain_init(grid_module_ain_buffer_size, 5, 14, 8);		
	grid_led_init(grid_module_led_buffer_size);

	spi_m_dma_get_io_descriptor(&GRID_LED, &io2);
	spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
		


	grid_rx_timout_init();
	
		
	grid_sys_uart_init();
	
	grid_rx_dma_init();	
	


	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);

	// ADC SETUP	
		
	if (grid_sys_get_hwcfg() == GRID_MODULE_P16_RevB){
						
	}
		
	if (grid_sys_get_hwcfg() == GRID_MODULE_B16_RevB){
			
		grid_adc_set_config(0, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(1, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(2, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(3, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(4, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(5, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(6, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(7, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(8, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(9, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(10, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(11, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(12, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(13, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(14, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(15, GRID_ADC_CFG_REVERSED, 1);
			
		grid_adc_set_config(0, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(1, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(2, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(3, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(4, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(5, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(6, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(7, GRID_ADC_CFG_BINARY, 1);		
		grid_adc_set_config(8, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(9, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(10, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(11, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(12, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(13, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(14, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(15, GRID_ADC_CFG_BINARY, 1);
			
	}
		
	if (grid_sys_get_hwcfg() == GRID_MODULE_PBF4_RevA){
			
		grid_adc_set_config(0, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(1, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(2, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(3, GRID_ADC_CFG_REVERSED, 1);
					
		grid_adc_set_config(12, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(13, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(14, GRID_ADC_CFG_REVERSED, 1);
		grid_adc_set_config(15, GRID_ADC_CFG_REVERSED, 1);
			
		grid_adc_set_config(12, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(13, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(14, GRID_ADC_CFG_BINARY, 1);
		grid_adc_set_config(15, GRID_ADC_CFG_BINARY, 1);
					
	}
				
		
	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_start_conversion(&ADC_0);
				
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_1);
	adc_async_enable_channel(&ADC_1, 0);
	adc_async_start_conversion(&ADC_1);
		
	

	
	// ===================== USART SETUP ========================= //
	
	//usart_async_register_callback(&GRID_AUX, USART_ASYNC_TXC_CB, tx_cb_GRID_AUX);
	/*usart_async_register_callback(&GRID_AUX, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&GRID_AUX, USART_ASYNC_ERROR_CB, err_cb);*/
	
	usart_async_get_io_descriptor(&GRID_AUX, &io);
	usart_async_enable(&GRID_AUX);


	// GRID_LED Library NEW NEW NEW NEW
	
		
		
}


#endif
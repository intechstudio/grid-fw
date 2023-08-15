#include "grid_d51_module.h"



static volatile struct grid_port PORT_N;
static volatile struct grid_port PORT_E;
static volatile struct grid_port PORT_S;
static volatile struct grid_port PORT_W;
static volatile struct grid_port PORT_U;
static volatile struct grid_port PORT_H;

static volatile char PORT_N_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_N_RX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_E_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_E_RX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_S_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_S_RX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_W_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_W_RX[GRID_BUFFER_SIZE] = {0};

static volatile char PORT_U_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_U_RX[GRID_BUFFER_SIZE] = {0};

static volatile char PORT_H_TX[GRID_BUFFER_SIZE] = {0};
static volatile char PORT_H_RX[GRID_BUFFER_SIZE] = {0};

// Define all of the peripheral interrupt callbacks

	
/* ============================== GRID_MODULE_INIT() ================================ */


void grid_module_common_init(void){

	GRID_PORT_N = &PORT_N;
	GRID_PORT_E = &PORT_E;
	GRID_PORT_S = &PORT_S;
	GRID_PORT_W = &PORT_W;
	GRID_PORT_U = &PORT_U;
	GRID_PORT_H = &PORT_H;

	GRID_PORT_N->tx_buffer.buffer_storage = PORT_N_TX;
	GRID_PORT_N->rx_buffer.buffer_storage = PORT_N_RX;
	GRID_PORT_E->tx_buffer.buffer_storage = PORT_E_TX;
	GRID_PORT_E->rx_buffer.buffer_storage = PORT_E_RX;
	GRID_PORT_S->tx_buffer.buffer_storage = PORT_S_TX;
	GRID_PORT_S->rx_buffer.buffer_storage = PORT_S_RX;
	GRID_PORT_W->tx_buffer.buffer_storage = PORT_W_TX;
	GRID_PORT_W->rx_buffer.buffer_storage = PORT_W_RX;

	GRID_PORT_U->tx_buffer.buffer_storage = PORT_U_TX;
	GRID_PORT_U->rx_buffer.buffer_storage = PORT_U_RX;
	GRID_PORT_H->tx_buffer.buffer_storage = PORT_H_TX;
	GRID_PORT_H->rx_buffer.buffer_storage = PORT_H_RX;

	
	printf("Common init done\r\n");	

	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);

	// set priorities for all of the UI related interrupts

	grid_d51_nvic_set_interrupt_priority(ADC0_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC0_1_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC1_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(ADC1_1_IRQn, 1); 

	grid_d51_nvic_set_interrupt_priority(SERCOM3_0_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(SERCOM3_1_IRQn, 1); 
	grid_d51_nvic_set_interrupt_priority(SERCOM3_2_IRQn, 1); // SERCOM3_2_IRQn handles reading encoders
	grid_d51_nvic_set_interrupt_priority(SERCOM3_3_IRQn, 1); 

	// disable ui interrupts
	grid_d51_nvic_set_interrupt_priority_mask(1);

	
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevC){
		printf("Init Module: PO16\r\n");
		grid_module_po16_init();
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevC ){
		printf("Init Module: BU16\r\n");
		grid_module_bu16_init();
	
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevA){
		printf("Init Module: PBF4\r\n");					
		grid_module_pbf4_init();			
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD ){
		printf("Init Module: EN16\r\n");
		grid_module_en16_init();	
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ){
		printf("Init Module: EN16 ND\r\n");
		grid_module_en16_init();	
	}		
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD ){
		printf("Init Module: EF44\r\n");
		grid_module_ef44_init();	
	}	
	else{
		printf("Init Module: Unknown Module\r\n");
	}

	printf("Model init done\r\n");


	



	grid_port_init_all();
	grid_d51_uart_init();

	grid_sys_set_bank(&grid_sys_state, 0);

	grid_d51_nvm_init(&grid_d51_nvm_state, &FLASH_0);
	
		
}



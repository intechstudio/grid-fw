

#include "grid/grid_module.h"

#include <atmel_start.h>
#include "atmel_start_pins.h"




#include <hal_qspi_dma.h>
#include "flash/spi_nor_flash.h"
#include "flash/n25q256a.h"


#include <stdio.h>


#include <hpl_reset.h>





volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;


/// TASK SWITCHER

#define GRID_TASK_NUMBER 8

enum grid_task{
	
	GRID_TASK_IDLE,
	GRID_TASK_UNDEFINED,
	GRID_TASK_RECEIVE,
	GRID_TASK_REPORT,
	GRID_TASK_INBOUND,
	GRID_TASK_OUTBOUND,
	GRID_TASK_LED,
	GRID_TASK_ALERT,
	
	
};

struct grid_task_model{
	
	uint8_t status;
	enum grid_task current_task;
	
	uint32_t timer[GRID_TASK_NUMBER];
	
};


struct grid_task_model grid_task_state;

enum grid_task grid_task_enter_task(struct grid_task_model* mod, enum grid_task next_task){
	
	
	enum grid_task previous_task = mod->current_task;
	mod->current_task = next_task;
	return previous_task;
	
}

grid_task_leave_task(struct grid_task_model* mod, enum grid_task previous_task){
	
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










void grid_port_reset_receiver(struct grid_port* por){
	
	usart_async_disable(por->usart);
	
	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;
	por->partner_status = 0;
	por->rx_double_buffer_timeout = 0;
	grid_sys_port_reset_dma(por);
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[por->rx_double_buffer_seek_start_index] = 0;
	}
	
	usart_async_enable(por->usart);
	
}



void grid_port_receive_task(struct grid_port* por){
		

		
	// THERE IS ALREADY DATA, PROCESS THAT FIRST
	if	(por->rx_double_buffer_status != 0){
		return;
	}
	
	
		
	if (por->rx_double_buffer_timeout > 1000){
		
		if (por->partner_status == 1){
			
			
			printf("{\"type\":\"PORT\", \"data\": [\"Timeout: Disconnect\"]}\r\n");
			
			
			printf("{\"type\":\"ERROR\", \"data\": [\"Buffer Overrun\"]}\r\n");
			grid_port_reset_receiver(por);	
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 2, 200);
		}
		else{
		
			if (por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0){
				// Ready to receive
			}
			else{
				grid_port_reset_receiver(por);
			}
			
		}		
			
	}
	else{
		
		por->rx_double_buffer_timeout++;
	}		
		

	for(uint32_t i = 0; i<490; i++){
		
		if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 10){ // \n
	
			por->rx_double_buffer_status = 1;
			por->rx_double_buffer_timeout = 0;
			
			return;
		}
		else if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 0){


			return;
		}
		
		
		// Buffer overrun error
		if (por->rx_double_buffer_seek_start_index == por->rx_double_buffer_read_start_index-1){
			gpio_set_pin_level(UI_PWR_EN, false);
			printf("{\"type\":\"TRAP\", \"data\": [\"TRAP1\"]}\r\n");
			while(1){}
		}
		
		// Buffer overrun error
		if (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE-1 && por->rx_double_buffer_read_start_index == 0){			
			gpio_set_pin_level(UI_PWR_EN, false);
			printf("{\"type\":\"TRAP\", \"data\": [\"TRAP2\"]}\r\n");
			while(1){}
		}
		
		
		if (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE -1)%GRID_DOUBLE_BUFFER_RX_SIZE] !=0){	
				
			printf("{\"type\":\"ERROR\", \"data\": [\"Buffer Overrun\"]}\r\n");
			grid_port_reset_receiver(por);	
			break;
	
		}
		
		
		grid_sys_alert_set_alert(&grid_sys_state, 50, 0, 0, 2, 200); // red

		if (por->rx_double_buffer_seek_start_index < GRID_DOUBLE_BUFFER_RX_SIZE-1){
			
			por->rx_double_buffer_timeout = 0;
			por->rx_double_buffer_seek_start_index++;			
		}
		else{
			
			por->rx_double_buffer_timeout = 0;
			por->rx_double_buffer_seek_start_index=0;
		}
		
	}
			
	
}

void grid_port_receive_decode(struct grid_port* por, uint32_t startcommand, uint32_t len){
	
	
	printf("{\"type\":\"PORT\", \"data\": [\"Decode\"]}\r\n");
	


	uint8_t error_flag = 0;
	uint8_t checksum_calculated = 0;
	uint8_t checksum_received = 0;
				
	// Copy data from cyrcular buffer to temporary linear array;
	uint8_t* message;
	
	uint32_t length = len;
	uint8_t buffer[length];			

				
	// Store message in temporary buffer (MAXMSGLEN = 250 character)
	for (uint32_t i = 0; i<length; i++){
		buffer[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE]=0;
	}
	
	message = &buffer[0];
	
	// Clear data from rx double buffer	
	for (uint32_t i = 0; i<length; i++){
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
	}
		
	uint32_t readstartindex = por->rx_double_buffer_read_start_index; 
		
	por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length)%GRID_DOUBLE_BUFFER_RX_SIZE;
	por->rx_double_buffer_seek_start_index =  por->rx_double_buffer_read_start_index;
		
	por->rx_double_buffer_status = 0;
		
	// Correct the incorrect frame start location
 	for (uint32_t i = 1; i<length; i++){
				
 		if (buffer[i] == GRID_MSG_START_OF_HEADING){
			 
// 			printf("{\"type\":\"FRAMEERROR\", \"location\":\"%d\" , \"data\": [", readstartindex);
// 				
// 			for(uint8_t j = 0; j<length; j++){
// 			
// 			
// 				printf("\"%d\"", message[j]);
// 			
// 				if (j != length-1){
// 					printf(", ");
// 				}
// 			
// 			}
// 			
// 			printf("]}\r\n");
			 
			 
			 
 			length -= i;
 			message = &buffer[i];
			 
			printf("{\"type\": \"WARNING\", \"data\": [\"Frame Start Offset\"]}\r\n");		
				

	
			
 		}
 		
 	}				
				
	// frame validator
	if (message[0] == 1 && message [length-1] == 10){
					
		checksum_received = grid_msg_checksum_read(message, length);
					
		checksum_calculated = grid_msg_checksum_calculate(message, length);
					
		// checksum validator
		if (checksum_calculated == checksum_received && error_flag == 0){
						
			if (message[1] == GRID_MSG_BROADCAST){ // Broadcast message
								
				// Read the received id age values	
				uint8_t received_id  = grid_msg_get_id(message);;			
				uint8_t received_age = grid_msg_get_age(message);
				
				// Read the received X Y values (SIGNED INT)				
				int8_t received_dx  = grid_msg_get_dx(message) - GRID_SYS_DEFAULT_POSITION;
				int8_t received_dy  = grid_msg_get_dy(message) - GRID_SYS_DEFAULT_POSITION;

				// DO THE DX DY AGE calculations
				
				uint8_t updated_id  = received_id;
					
				int8_t rotated_dx = 0;
				int8_t rotated_dy = 0;

				// APPLY THE 2D ROTATION MATRIX
				
				if (por->partner_fi == 0){ // 0 deg		
					rotated_dx  += received_dx;
					rotated_dy  += received_dy;
				}
				else if(por->partner_fi == 1){ // 90 deg
					rotated_dx  -= received_dy;
					rotated_dy  += received_dx;
				}
				else if(por->partner_fi == 2){ // 180 deg
					rotated_dx  -= received_dx;
					rotated_dy  -= received_dy;
				}
				else if(por->partner_fi == 3){ // 270 deg
					rotated_dx  += received_dy;
					rotated_dy  -= received_dx;
				}
				else{				
					// TRAP INVALID MESSAGE			
				}
								
				uint8_t updated_dx = rotated_dx + GRID_SYS_DEFAULT_POSITION + por->dx;
				uint8_t updated_dy = rotated_dy + GRID_SYS_DEFAULT_POSITION + por->dy;
				
				
							
				uint8_t updated_age = received_age;
								
				
				// Update message with the new values

				grid_msg_set_id(message, updated_id);
				grid_msg_set_dx(message, updated_dx);
				grid_msg_set_dy(message, updated_dy);
				grid_msg_set_age(message, updated_age);
				
				uint32_t fingerprint = updated_id*256*256*256 + updated_dx*256*256 + updated_dy*256 + updated_age;
																				
				if (0 == grid_msg_find_recent(&grid_sys_state, fingerprint)){
					// WE HAVE NOT HEARD THIS MESSAGE BEFORE
										
					// Recalculate and update the checksum
														
					grid_msg_checksum_write(message, length, grid_msg_checksum_calculate(message, length));
					

					// IF WE CAN STORE THE MESSAGE IN THE RX BUFFER
					if (grid_buffer_write_init(&por->rx_buffer, length)){
										
										
										
						for (uint8_t i=0; i<length; i++){
							
							grid_buffer_write_character(&por->rx_buffer, message[i]);
							
						}
										
						grid_buffer_write_acknowledge(&por->rx_buffer);
										
						//grid_port_process_inbound(por);
																		
						grid_msg_push_recent(&grid_sys_state, fingerprint);										
										
					}
					
					
					
				}
				else{
					// WE ALREADY HEARD THIS MESSAGE							
					//grid_sys_alert_set_alert(&grid_sys_state, 50, 50, 50, 2, 200); // WHITE
								
				}
				
				

		
// 				uint32_t response_length = strlen(response);
// 		
// 				if(grid_buffer_write_init(&por->tx_buffer, response_length)){
// 						
// 			
// 					uint8_t checksum = grid_msg_get_checksum(response, response_length);
// 					grid_msg_set_checksum(response, response_length, checksum);
// 						
// 					for (uint32_t i=0; i<response_length; i++)
// 					{
// 						grid_buffer_write_character(&por->tx_buffer, response[i]);
// 					}
// 						
// 					grid_buffer_write_acknowledge(&por->tx_buffer);
// 													
// 				}
					
			}
			else if (message[1] == GRID_MSG_DIRECT){ // Direct Message
												
				//process direct message
							
				if (message[2] == GRID_MSG_ACKNOWLEDGE){				

					//grid_sys_alert_set_alert(&grid_sys_state, 30, 30, 30, 0, 250); // LIGHT WHITE PULSE
				}
				else if (message[2] == GRID_MSG_NACKNOWLEDGE){
					grid_sys_alert_set_alert(&grid_sys_state, 50, 0, 0, 0, 250); // LIGHT RED PULSE
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_MSG_CANCEL){
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_MSG_BELL){
								
								
					if (por->partner_status == 0){
						
			
						printf("{\"type\":\"PORT\", \"data\": [\"Connect: Connect\"]}\r\n");
						// CONNECT
						por->partner_fi = (message[3] - por->direction + 6)%4;
						por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-12], 8, error_flag);
						por->partner_status = 1;
						
						grid_sys_state.age = grid_sys_rtc_get_time(&grid_sys_state);
						grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 2, 200); // GREEN
						
						// SEND OUT CURRENT BANK NUMBER IF IT IS INITIALIZED
						if (grid_sys_state.bank_select!=255){
							struct grid_ui_model* mod = &grid_ui_state;
							grid_sys_write_hex_string_value(&mod->report_array[GRID_REPORT_INDEX_MAPMODE].payload[7], 2, grid_sys_state.bank_select);
							grid_report_sys_set_changed_flag(mod, 0);												
						}

															
					}
					else{
						// VALIDATE
						uint8_t validator = 1;
						validator &= (por->partner_fi == ((message[3] - por->direction + 6)%4));
						volatile uint32_t debug = grid_sys_read_hex_string_value(&message[length-12], 8, error_flag);
						volatile uint32_t debug2 = por->partner_hwcfg;
												
						validator &= (por->partner_hwcfg == debug);									
									
						if (validator == 0){
							//FAILED, DISCONNECT
							por->partner_status = 0;	
							grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 2, 200); // WHITE
							
							printf("{\"type\":\"PORT\", \"data\": [\"Connect: Disconnect\"]}\r\n");
										
						}
						else{
							
							
							printf("{\"type\":\"PORT\", \"data\": [\"Connect: Validate\"]}\r\n");
							//OK
							//grid_sys_alert_set_alert(&grid_sys_state, 6, 6, 6, 0, 200); // LIGHT WHITE
							
						}
									
									
					}
								
								
				}
																		
			}
			else{ // Unknown Message Type
					
				grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200); // RED SHORT
							
			}
						

						
		}
		else{
			// INVALID CHECKSUM

			if (error_flag != 0){		
				//usart_async_disable(&USART_EAST);
				grid_sys_alert_set_alert(&grid_sys_state, 20, 0, 0, 1, 200); // PURPLE BLINKY
				//usart_async_enable(&USART_EAST);
			}	
			else{
				
				grid_sys_alert_set_alert(&grid_sys_state, 20, 0, 255, 1, 200); // BLUE BLINKY
				
				
			}
		
	
		}
				

	}
	else{
		// frame error
		
		grid_sys_alert_set_alert(&grid_sys_state, 0, 0, 20, 2, 200); // BLUE BLINKY	
	
// 		printf("{\"type\":\"FRAMEERROR\", \"data\": [");
// 				
// 		for(uint8_t i = 0; i<length; i++){
// 			
// 			
// 			printf("\"%d\"", message[i]);
// 			
// 			if (i != length-1){
// 				printf(", ");
// 			}
// 			
// 		}
// 			
// 		printf("]}\r\n");


		printf("{\"type\": \"ERROR\", \"data\": [\"Frame Error\"]}\r\n");
	}
		
		
		




	
	return;
	
}

void grid_port_receive_complete_task(struct grid_port* por){
	
	if (por->usart_error_flag == 1){
		
		por->usart_error_flag = 0;
		
		grid_port_reset_receiver(por);			
		
		printf("{\"type\": \"ERROR\", \"data\": [\"Parity Error\"]}\r\n");
		
	}
	

	///////////////////// PART 1
	
	grid_port_receive_task(por);	
	
	
	////////////////// PART 2
	
	if (por->rx_double_buffer_status == 0){
		return;
	}

	
	uint32_t length = 0;
	
	if (por->rx_double_buffer_read_start_index < por->rx_double_buffer_seek_start_index){
		length = por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	else{
		length = GRID_DOUBLE_BUFFER_RX_SIZE + por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	
		
	grid_port_receive_decode(por, por->rx_double_buffer_read_start_index, length);
			

	
	por->rx_double_buffer_status = 0;
	
	
	
}




static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_report;
static struct timer_task RTC_Scheduler_heartbeat;

static void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
{
	// [2...5] is ping report descriptor
	pingflag++;
	
	switch (pingflag%4)
	{
		case 0:
			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_NORTH);
			break;
		case 1:
			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_EAST);
			break;
		case 2:
			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_SOUTH);
			break;
		case 3:
			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_WEST);
			break;
	}
	
}

static void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{
	grid_sys_rtc_tick_time(&grid_sys_state);	
	grid_task_timer_tick(&grid_task_state);
	
		// HANDLE MAPMODE CHANGES
	struct grid_ui_model* mod = &grid_ui_state;
		
	//CRITICAL_SECTION_ENTER()

	uint8_t mapmode_value = gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0]){
			
		uint8_t value;
			
		if (mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] == 0){
				
			mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] = 1;
				
		}
		else{
				
			mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] = 0;
				
			grid_sys_state.bank_select = (grid_sys_state.bank_select+1)%4;
			value = grid_sys_state.bank_select;
 			grid_sys_write_hex_string_value(&mod->report_array[GRID_REPORT_INDEX_MAPMODE].payload[7], 2, grid_sys_state.bank_select);
 			grid_report_sys_set_changed_flag(mod, GRID_REPORT_INDEX_MAPMODE);
		}
			
			
			

	}

}

static void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{
	grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_HEARTBEAT);
}

volatile uint8_t scheduler_report_flag = 0;
static void RTC_Scheduler_report_cb(const struct timer_task *const timer_task)
{
	scheduler_report_flag = 1;
	
}


#define RTC1SEC 16384

void init_timer(void)
{
	
		
	//RTC_Scheduler_ping.interval = RTC1SEC/20; //50ms
	RTC_Scheduler_ping.interval = RTC1SEC/20;
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_heartbeat.interval = RTC1SEC;
	RTC_Scheduler_heartbeat.cb       = RTC_Scheduler_heartbeat_cb;
	RTC_Scheduler_heartbeat.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_report.interval = RTC1SEC/10;
	RTC_Scheduler_report.cb       = RTC_Scheduler_report_cb;
	RTC_Scheduler_report.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_realtime.interval = 1;
	RTC_Scheduler_realtime.cb       = RTC_Scheduler_realtime_cb;
	RTC_Scheduler_realtime.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_heartbeat);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
	
	timer_start(&RTC_Scheduler);
	
}

struct io_descriptor *io;


//  ========================== NOR FLASH =================================  //

#define BUFFER_SIZE 64
/** Size of minimum erase block */
#define QSPI_ERBLK (4 * 1024)
/** Size of data to erase (blocks to cover writing area) */
#define QSPI_ERSIZE ((BUFFER_SIZE + QSPI_ERBLK - 1) & ~(QSPI_ERBLK - 1))

/* Declare  Tx and Rx buffer and initialize to 0 */
volatile uint8_t tx_buffer[BUFFER_SIZE] = {0};
volatile uint8_t rx_buffer[BUFFER_SIZE] = {0};

static struct n25q256a SPI_NOR_FLASH_0_descr;

struct spi_nor_flash *SPI_NOR_FLASH_0;

volatile bool is_corrupted = false;		

#define CONF_SPI_NOR_FLASH_0_QUAD_MODE 1

#define FLASH_IO0 PA08
#define FLASH_IO1 PA09
#define FLASH_IO2 PA10
#define FLASH_IO3 PA11

#define FLASH_CLK PB10
#define FLASH_CS  PB11

void QSPI_INSTANCE_exit_xip(void)
{
	gpio_set_pin_function(FLASH_IO0, 0);
	gpio_set_pin_function(FLASH_CS, 0);
	gpio_set_pin_function(FLASH_CLK, 0);

	gpio_set_pin_direction(FLASH_IO0, GPIO_DIRECTION_OUT);
	gpio_set_pin_direction(FLASH_CS, GPIO_DIRECTION_OUT);
	gpio_set_pin_direction(FLASH_CLK, GPIO_DIRECTION_OUT);

	gpio_set_pin_level(FLASH_IO0, true);
	gpio_set_pin_level(FLASH_CS, false);
	gpio_set_pin_level(FLASH_CLK, false);

	delay_us(1);

	for (int i = 0; i < 7; i++) {
		gpio_set_pin_level(FLASH_CLK, true);
		delay_us(1);
		gpio_set_pin_level(FLASH_CLK, false);
		delay_us(1);
	}

	gpio_set_pin_level(FLASH_CS, true);
	delay_us(1);
	QSPI_INSTANCE_PORT_init();
}


/**
 * \brief Initialize NOR Flash memory
 */
void spi_nor_flash_init(void)
{

	qspi_dma_enable(&QSPI_INSTANCE);
	SPI_NOR_FLASH_0 = n25q256a_construct(
	    &SPI_NOR_FLASH_0_descr.parent, &QSPI_INSTANCE, QSPI_INSTANCE_exit_xip, CONF_SPI_NOR_FLASH_0_QUAD_MODE);
}

void spi_nor_flash_test(void){
	
	printf("QSPI Program Started\n\r");
	/* Initialize Tx buffer */
	for (int i = 0; i < BUFFER_SIZE; i++) {
		tx_buffer[i] = (uint8_t)i;
	}

	/* Erase flash memory */
	if (ERR_NONE == SPI_NOR_FLASH_0->interface->erase(SPI_NOR_FLASH_0, 0, QSPI_ERSIZE)) {
		printf("Flash erase successful\n\r");
	}

	/* Write data to flash memory */
	if (ERR_NONE == SPI_NOR_FLASH_0->interface->write(SPI_NOR_FLASH_0, (uint8_t *)tx_buffer, 0, BUFFER_SIZE)) {
		printf("Flash write successful\n\r");
	}

	/* Read data from flash memory */
	if (ERR_NONE == SPI_NOR_FLASH_0->interface->read(SPI_NOR_FLASH_0, (uint8_t *)rx_buffer, 0, BUFFER_SIZE)) {
		printf("Flash read successful\n\r");
	}
		
	/* Data verification */
	for (int i = 0; i < BUFFER_SIZE; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			is_corrupted = true;
			printf("Flash data verification failed.\n\r");
		}
	}

	if (!is_corrupted) {
		printf("Write - Read is successful in QSPI Flash memory.\n\r");
	}
	
}


/* DMA Transfer complete callback */
static void qspi_xfer_complete_cb(struct _dma_resource *resource)
{
	/* Pull Up Chip select line*/
	hri_qspi_write_CTRLA_reg(QSPI, QSPI_CTRLA_ENABLE | QSPI_CTRLA_LASTXFER);
	

}




int main(void)
{



	atmel_start_init();	

	
	printf("Initialization\r\n");
		
	#ifdef UNITTEST	
		#include "grid/grid_unittest.h"
		grid_unittest_start();	
		
		grid_sys_unittest();	
		grid_sys_unittest();	
	
		printf(" Unit Test Finished\r\n");
		
		while (1)
		{
		}
		
	#endif
	
	#ifdef HARDWARETEST
	
		#include "grid/grid_hardwaretest.h"
		
		grid_hardwaretest_main();
		
		while (1)
		{
		}
		
	#endif

	
// 	uint32_t flash_length = flash_get_total_pages(&FLASH_0);
// 	
// 	flash_lock(&FLASH_0, 0x00000000, flash_length);
	
	
//  	wdt_set_timeout_period(&WDT_0, 1000, 4096);
//  	wdt_enable(&WDT_0);
//  	wdt_feed(&WDT_0
		
//	wdt_disable(&WDT_0);
	

	//TIMER_0_example2();
	#include "usb/class/midi/device/audiodf_midi.h"
	audiodf_midi_init();


	composite_device_start();

	grid_module_common_init();


	uint32_t loopstart = 0;

					
	uint32_t hwtype = grid_sys_get_hwcfg();
	
	for (uint8_t i = 0; i<grid_led_get_led_number(&grid_led_state); i++)
	{

		if (hwtype == GRID_MODULE_EN16_RevA){	
			grid_led_set_min(&grid_led_state, i, 0, 0, 0, 255);
			grid_led_set_mid(&grid_led_state, i, 0, 0, 5, 0);
			grid_led_set_max(&grid_led_state, i, 0, 255, 0, 0);
		}
	}
		
		
		

	grid_sys_bank_select(&grid_sys_state, 255);
	
	init_timer();
	
	uint32_t loopcounter = 0;

	


	


	/* Register DMA complete Callback and initialize NOR flash */
 	//qspi_dma_register_callback(&QSPI_INSTANCE, QSPI_DMA_CB_XFER_DONE, qspi_xfer_complete_cb);	
 	//spi_nor_flash_init();	
	 
	 
	 
 	//spi_nor_flash_test();
	

	printf("Entering Main Loop\r\n");
	
	uint8_t usb_init_variable = 0;

	while (1) {
		
				
		grid_task_enter_task(&grid_task_state, GRID_TASK_UNDEFINED);
		
		
		if (usb_init_variable == 0){
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{
				printf("USB Connected\r\n");
				grid_sys_bank_select(&grid_sys_state, 0);
				usb_init_variable = 1;
			}
			
		}
	
		
		//grid_selftest(loopcounter);
		
		loopcounter++;
	
		loopstart = grid_sys_rtc_get_time(&grid_sys_state);
		
		if (scheduler_report_flag){
			
			scheduler_report_flag=0;
		
			uint32_t task_val[GRID_TASK_NUMBER] = {0};
				
			for(uint8_t i = 0; i<GRID_TASK_NUMBER; i++){
	
				task_val[i] = grid_task_timer_read(&grid_task_state, i);
			
			}
			grid_task_timer_reset(&grid_task_state);
			
				
			printf("{\"type\":\"TASK\", \"data\": [");
				
			for(uint8_t i = 0; i<GRID_TASK_NUMBER; i++){
			
			
				printf("\"%d\"", task_val[i]);
			
				if (i != GRID_TASK_NUMBER-1){
					printf(", ");
				}
			
			}
			
			printf("]}\r\n");
			
			printf("{\"type\":\"LOOP\", \"data\": [\"%d\"]}\r\n", loopcounter);
			loopcounter = 0;
		
			
		}
		
		
		
							
		grid_task_enter_task(&grid_task_state, GRID_TASK_RECEIVE);

		// CHECK RX BUFFERS
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);
					
					
					
		/* ========================= GRID REPORT TASK ============================= */
		grid_task_enter_task(&grid_task_state, GRID_TASK_REPORT);

			

		// COOLDOWN DELAY IMPLEMENTED INSIDE
		grid_port_process_ui(&GRID_PORT_U);



		/* ========================= GRID INBOUND TASK ============================= */						
		grid_task_enter_task(&grid_task_state, GRID_TASK_INBOUND);	
		
		// Copy data from UI_RX to HOST_TX & north TX AND STUFF
		grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback
		
		grid_port_process_inbound(&GRID_PORT_N, 0);		
		grid_port_process_inbound(&GRID_PORT_E, 0);		
		grid_port_process_inbound(&GRID_PORT_S, 0);		
		grid_port_process_inbound(&GRID_PORT_W, 0);						
		
		
		
		/* ========================= GRID OUTBOUND TASK ============================= */		
		grid_task_enter_task(&grid_task_state, GRID_TASK_OUTBOUND);
		
		// If previous xfer is completed and new data is available then move data from txbuffer to txdoublebuffer and start new xfer.
		grid_port_process_outbound_usart(&GRID_PORT_N);
		grid_port_process_outbound_usart(&GRID_PORT_E);
		grid_port_process_outbound_usart(&GRID_PORT_S);
		grid_port_process_outbound_usart(&GRID_PORT_W);
		
		// Translate grid messages to usb messages and xfer them to the host
		grid_port_process_outbound_usb(&GRID_PORT_H);
		
		// Translate grid messages to ui commands (LED)
		grid_port_process_outbound_ui(&GRID_PORT_U);


		/* ========================= GRID ALERT TASK ============================= */		
		grid_task_enter_task(&grid_task_state, GRID_TASK_ALERT);	
		
			
		if (grid_sys_state.alert_state){
			
			grid_sys_state.alert_state--;
	
			if (grid_sys_alert_read_color_changed_flag(&grid_sys_state)){
				
				grid_sys_alert_clear_color_changed_flag(&grid_sys_state);			
				
				uint8_t color_r   = grid_sys_alert_get_color_r(&grid_sys_state);
				uint8_t color_g   = grid_sys_alert_get_color_g(&grid_sys_state);
				uint8_t color_b   = grid_sys_alert_get_color_b(&grid_sys_state);
				
				for (uint8_t i=0; i<grid_led_get_led_number(&grid_led_state); i++){
				

				
						grid_led_set_min(&grid_led_state, i, 1, color_r*0   , color_g*0   , color_b*0);
						grid_led_set_mid(&grid_led_state, i, 1, color_r*0.5 , color_g*0.5 , color_b*0.5);
						grid_led_set_max(&grid_led_state, i, 1, color_r*1   , color_g*1   , color_b*1);
						
		

					
				}
		
			}
			
			uint8_t intensity = grid_sys_alert_get_color_intensity(&grid_sys_state);
	
			for (uint8_t i=0; i<grid_led_state.led_number; i++){	
				//grid_led_set_color(i, 0, 255, 0);	
		
				grid_led_set_phase(&grid_led_state, i, 1, intensity);
								
			}
			
			
		}
		
		grid_task_enter_task(&grid_task_state, GRID_TASK_LED);
	
		grid_led_tick(&grid_led_state);
	

		
		if (loopcounter%1 == 0){
			
			grid_led_render_all(&grid_led_state);	
			
						
	 		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
	
	 		}
			grid_led_hardware_start_transfer(&grid_led_state);
		}		
		
		
	
	
	
	
		grid_task_enter_task(&grid_task_state, GRID_TASK_IDLE);


		// IDLETASK
		while(grid_sys_rtc_get_elapsed_time(&grid_sys_state, loopstart) < RTC1SEC/1000){
			
			delay_us(1);
			
		}	
		
		grid_task_enter_task(&grid_task_state, GRID_TASK_UNDEFINED);		

		

	}//WHILE
	
	
	
}//MAIN



#include "grid/grid_module.h"

#include <atmel_start.h>
#include "atmel_start_pins.h"




#include <hal_qspi_dma.h>
#include "flash/spi_nor_flash.h"
#include "flash/n25q256a.h"


#include <stdio.h>


#include <hpl_reset.h>


static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;


volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;
volatile uint8_t pingflag_active = 0;


void grid_selftest(uint32_t loop){
	
	// SELFTEST()
	if (1){
		
		//INIT PORTS
		if (1){
			
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
			
		}
		
		
		if (loop%1024 == 0){
			
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
		if (loop%1024 == 512-100){
			
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
		if (loop%1024 == 512){
			
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
		if (loop%1024 == 1024-100){
			
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
		
	} // END OF SELFTEST
	
	
}


void grid_port_receive_task(struct grid_port* por){
		

	// THERE IS ALREADY DATA, PROCESS THAT FIRST
	if	(por->rx_double_buffer_status != 0){
		return;
	}
	
	
		
	if (por->rx_double_buffer_timeout > 2000){
		
		if (por->partner_status == 1){
			
			por->rx_double_buffer_seek_start_index = 0;
			por->rx_double_buffer_read_start_index = 0;
			por->partner_status = 0;
			por->rx_double_buffer_timeout =0;
			grid_sys_port_reset_dma(por);
			
// 			grid_sys_state.error_code = 7; // WHITE
// 			grid_sys_state.error_style = 2; // CONST
// 			grid_sys_state.error_state = 200; // CONST
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 2, 200);
		}
		else{
		
			por->rx_double_buffer_seek_start_index = 0;
			por->rx_double_buffer_read_start_index = 0;
			grid_sys_port_reset_dma(por);
		
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
		
		


		if (por->rx_double_buffer_seek_start_index < GRID_DOUBLE_BUFFER_RX_SIZE-1){
			por->rx_double_buffer_seek_start_index++;			
		}
		else{
			por->rx_double_buffer_seek_start_index=0;
		}
		
	}
		
	
	
}

void grid_port_receive_decode(struct grid_port* por, uint32_t startcommand, uint32_t length){
	
	uint8_t response[10];
	
	response[0] = GRID_MSG_START_OF_HEADING;
	response[1] = GRID_MSG_DIRECT;
	response[2] = GRID_MSG_NACKNOWLEDGE;
	response[3] = GRID_MSG_END_OF_TRANSMISSION;
	response[4] = '0'; //checksum
	response[5] = '0'; //checksum
	response[6] = '\n';
	response[7] = 0;
	response[8] = 0;
	response[9] = 0;

	uint8_t error_flag = 0;
	uint8_t checksum_calculated = 0;
	uint8_t checksum_received = 0;
				
	// Copy data from cyrcular buffer to te3mporary linear array;
	uint8_t message[length];
				
	// MAXMSGLEN = 250 character
	for (uint32_t i = 0; i<length; i++){
		message[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
	}
				
	// frame validator
	if (message[0] == 1 && message [length-1] == 10){
					
		checksum_received = grid_sys_read_hex_string_value(&message[length-3], 2, &error_flag);
					
		checksum_calculated = grid_msg_get_checksum(message, length);
					
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
														
					grid_msg_set_checksum(message, length, grid_msg_get_checksum(message, length));
					

					// IF WE CAN STORE THE MESSAGE IN THE RX BUFFER
					if (grid_buffer_write_init(&por->rx_buffer, length)){
										
										
										
						for (uint8_t i=0; i<length; i++){
							
							grid_buffer_write_character(&por->rx_buffer, message[i]);
							
						}
										
						grid_buffer_write_acknowledge(&por->rx_buffer);
										
						grid_port_process_inbound(por);
																		
						grid_msg_push_recent(&grid_sys_state, fingerprint);
																	
						response[2] = GRID_MSG_ACKNOWLEDGE;
										
										
					}
					
					
					
				}
				else{
					// WE ALREADY HEARD THIS MESSAGE					
					response[2] = GRID_MSG_ACKNOWLEDGE;							
					grid_sys_alert_set_alert(&grid_sys_state, 50, 50, 50, 2, 200); // WHITE
								
				}
				
				

		
				uint32_t response_length = strlen(response);
		
				if(grid_buffer_write_init(&por->tx_buffer, response_length)){
						
			
					uint8_t checksum = grid_msg_get_checksum(response, response_length);
					grid_msg_set_checksum(response, response_length, checksum);
						
					for (uint32_t i=0; i<response_length; i++)
					{
						grid_buffer_write_character(&por->tx_buffer, response[i]);
					}
						
					grid_buffer_write_acknowledge(&por->tx_buffer);
													
				}
					
			}
			else if (message[1] == GRID_MSG_DIRECT){ // Direct Message
												
				//process direct message
							
				if (message[2] == GRID_MSG_ACKNOWLEDGE){				

					grid_sys_alert_set_alert(&grid_sys_state, 30, 30, 30, 0, 250); // LIGHT WHITE PULSE
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
						// CONNECT
						por->partner_fi = (message[3] - por->direction + 6)%4;
						por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-12], 8, error_flag);
						por->partner_status = 1;
						
						grid_sys_state.age = grid_sys_rtc_get_time(&grid_sys_state);
						grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 2, 200); // GREEN
						
						// SEND OUT CURRENT BANK NUMBER IF IT IS INITIALIZED
						if (grid_sys_state.bank_select!=255){
							struct grid_ui_model* mod = &grid_ui_state;
							grid_sys_write_hex_string_value(&mod->report_array[0].payload[7], 2, grid_sys_state.bank_select);
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
										
						}
						else{
							//OK
							//grid_sys_alert_set_alert(&grid_sys_state, 6, 6, 6, 0, 200); // LIGHT WHITE
							
						}
									
									
					}
								
								
				}
																		
			}
			else{ // Invalid
					
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
				
				grid_sys_alert_set_alert(&grid_sys_state, 20, 0, 255, 1, 200); // PURPLE BLINKY
				
				
			}
		
	
		}
				

	}
	else{
		// frame error
		grid_sys_alert_set_alert(&grid_sys_state, 0, 0, 20, 2, 200); // BLUE BLINKY	

	}
		
		
		

	
	for (uint32_t i = 0; i<length; i++){
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
	}
		
	por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length)%GRID_DOUBLE_BUFFER_RX_SIZE;
	por->rx_double_buffer_seek_start_index =  por->rx_double_buffer_read_start_index;
	
	
	por->rx_double_buffer_status = 0;


	
	return;
	
}

void grid_port_receive_complete_task(struct grid_port* por){
	

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


static void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
{
	pingflag++;
	pingflag_active++;
}


static void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{
	grid_sys_rtc_tick_time(&grid_sys_state);
	
		// HANDLE MAPMODE CHANGES
	struct grid_ui_model* mod = &grid_ui_state;
		
	//CRITICAL_SECTION_ENTER()

	uint8_t report_index = 0;

	uint8_t mapmode_value = gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != mod->report_array[report_index].helper[0]){
			
		uint8_t value;
			
		if (mod->report_array[report_index].helper[0] == 0){
				
			mod->report_array[report_index].helper[0] = 1;
				
		}
		else{
				
			mod->report_array[report_index].helper[0] = 0;
				
			grid_sys_state.bank_select = (grid_sys_state.bank_select+1)%4;
			value = grid_sys_state.bank_select;
 			grid_sys_write_hex_string_value(&mod->report_array[report_index].payload[7], 2, grid_sys_state.bank_select);
 			grid_report_sys_set_changed_flag(mod, report_index);
		}
			
			
			

	}

}

#define RTC1SEC 16384

void init_timer(void)
{
	
		
	//RTC_Scheduler_ping.interval = RTC1SEC/20; //50ms
	RTC_Scheduler_ping.interval = RTC1SEC/20; //was /5: 200ms
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_realtime.interval = 1;
	RTC_Scheduler_realtime.cb       = RTC_Scheduler_realtime_cb;
	RTC_Scheduler_realtime.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
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
		
		
		
	gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
	gpio_set_pin_level(PIN_GRID_SYNC_1, false);	
	
	init_timer();
	
	uint32_t loopcounter = 0;

	
	//grid_sys_bank_select(&grid_sys_state, 255);

	


	/* Register DMA complete Callback and initialize NOR flash */
 	//qspi_dma_register_callback(&QSPI_INSTANCE, QSPI_DMA_CB_XFER_DONE, qspi_xfer_complete_cb);	
 	//spi_nor_flash_init();	
	 
	 
	 
 	//spi_nor_flash_test();
	

	printf("Entering Main Loop\r\n");
	
	uint8_t usb_init_variable = 0;

	while (1) {
		
		if (usb_init_variable == 0){
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{
				
				grid_sys_bank_select(&grid_sys_state, 0);
				usb_init_variable = 1;
			}
			
		}
	
		
		//grid_selftest(loopcounter);
		
		loopcounter++;
	
		loopstart = grid_sys_rtc_get_time(&grid_sys_state);
							
		
		/* ========================= PING ============================= */
		if (pingflag_active){
			
			if (pingflag%4 == 0){
				grid_sys_ping(&GRID_PORT_N);
			}
			if (pingflag%4 == 1){
				grid_sys_ping(&GRID_PORT_E);
			}
			if (pingflag%4 == 2){
				grid_sys_ping(&GRID_PORT_S);
			}
			if (pingflag%4 == 3){
				grid_sys_ping(&GRID_PORT_W);
			}
			pingflag_active = 0;
			
		}			

	


		// CHECK RX BUFFERS
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);

					
		/* ========================= UI_PROCESS_INBOUND ============================= */
			

		// COOLDOWN DELAY IMPLEMENTED INSIDE
		grid_port_process_ui(&GRID_PORT_U);


		
		grid_port_process_inbound(&GRID_PORT_U); // Copy data from UI_RX to HOST_TX & north TX AND STUFF

		grid_port_process_inbound(&GRID_PORT_N);		
		grid_port_process_inbound(&GRID_PORT_E);		
		grid_port_process_inbound(&GRID_PORT_S);		
		grid_port_process_inbound(&GRID_PORT_W);						
		
		/* ========================= GRID MOVE TASK ============================= */		

		
		grid_port_process_outbound_usart(&GRID_PORT_N);
		grid_port_process_outbound_usart(&GRID_PORT_E);
		grid_port_process_outbound_usart(&GRID_PORT_S);
		grid_port_process_outbound_usart(&GRID_PORT_W);
		
		grid_port_process_outbound_usb(&GRID_PORT_H); // Send data from HOST_TX through USB
		grid_port_process_outbound_ui(&GRID_PORT_U);

		
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
		
		
	
		grid_led_tick(&grid_led_state);
			
			
		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
			
		}
		
		grid_led_render_all(&grid_led_state);
				

					
		grid_led_hardware_start_transfer(&grid_led_state);
	
	
	
	

		// IDLETASK
		while(grid_sys_rtc_get_elapsed_time(&grid_sys_state, loopstart) < RTC1SEC/1000){
			
			delay_us(10);
			
		}
				
				

		

	}//WHILE
	
	
	
}//MAIN

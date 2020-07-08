

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
	
	struct grid_ui_report* stored_report = por->ping_report;
	grid_sys_write_hex_string_value(&stored_report->payload[8], 2, 255);
	grid_sys_write_hex_string_value(&stored_report->payload[6], 2, 255);
	grid_msg_checksum_write(stored_report->payload, stored_report->payload_length, grid_msg_checksum_calculate(stored_report->payload, stored_report->payload_length));
	
	
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
			
			
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Timeout Disconnect & Reset Receiver");
			
			grid_port_reset_receiver(por);	
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 0, 500);
		}
		else{
		
			if (por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0){
				// Ready to receive
			}
			else{
							
				GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Timeout & Reset Receiver");
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
			
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "rx_double_buffer overrun 1");
			
			grid_port_reset_receiver(por);
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200);
			return;	
		}
		
		// Buffer overrun error
		if (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE-1 && por->rx_double_buffer_read_start_index == 0){			
			
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "rx_double_buffer overrun 2");
			
			grid_port_reset_receiver(por);
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200);
			return;
		}
		
		
		if (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE -1)%GRID_DOUBLE_BUFFER_RX_SIZE] !=0){	
				
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "rx_double_buffer overrun 3");
			
			grid_port_reset_receiver(por);	
			
			grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200);
			return;
	
		}
		
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
				
 		if (buffer[i] == GRID_CONST_SOH){
			 			 		 
 			length -= i;
 			message = &buffer[i];
			 
			
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Frame Start Offset");
			
			
 		}
 		
 	}				
				
	// frame validator
	if (message[0] == GRID_CONST_SOH && message [length-1] == GRID_CONST_LF){
					
		checksum_received = grid_msg_checksum_read(message, length);
					
		checksum_calculated = grid_msg_checksum_calculate(message, length);
					
		// checksum validator
		if (checksum_calculated == checksum_received && error_flag == 0){
						
			if (message[1] == GRID_CONST_BRC){ // Broadcast message
								
				uint8_t error=0;
								
				// Read the received id age values	
				uint8_t received_id  = grid_msg_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_offset, &error);			
				uint8_t received_age = grid_msg_get_parameter(message, GRID_BRC_AGE_offset, GRID_BRC_AGE_length, &error);
				
				// Read the received X Y values (SIGNED INT)				
				int8_t received_dx  = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_SYS_DEFAULT_POSITION;
				int8_t received_dy  = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_SYS_DEFAULT_POSITION;
				
				uint8_t received_rot = grid_msg_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
				

				// DO THE DX DY AGE calculations
				
				uint8_t updated_id  = received_id;
					
				int8_t rotated_dx = 0;
				int8_t rotated_dy = 0;
				
				uint8_t updated_rot = (received_rot + por->partner_fi)%4;

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
				grid_msg_set_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_offset, updated_id, &error);
				grid_msg_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
				grid_msg_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);
				grid_msg_set_parameter(message, GRID_BRC_AGE_offset, GRID_BRC_AGE_length, updated_age, &error);
				grid_msg_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);	

				
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
			else if (message[1] == GRID_CONST_DCT){ // Direct Message
												
				//process direct message
							
				if (message[2] == GRID_CONST_ACK){				

					//grid_sys_alert_set_alert(&grid_sys_state, 30, 30, 30, 0, 250); // LIGHT WHITE PULSE
				}
				else if (message[2] == GRID_CONST_NAK){
					//grid_sys_alert_set_alert(&grid_sys_state, 50, 0, 0, 0, 250); // LIGHT RED PULSE
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_CAN){
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_BELL){
						
											
					// Handshake logic	
													
					uint8_t local_stored = 255; // I think this is my id
					uint8_t remote_stored = 255; // I think this is my neighbor's id
					uint8_t local_received = 255; // My neighbor thinks this is my id
					uint8_t remote_received = 255; // My neighbor thinks this is their id
					
					uint8_t* local_stored_location = NULL;
					uint8_t* remote_stored_location = NULL;
					
					struct grid_ui_model* mod = &grid_ui_state;
					
					struct grid_ui_report* stored_report = por->ping_report;
										
					
					local_stored = grid_sys_read_hex_string_value(&stored_report->payload[6], 2, error_flag);
					remote_stored = grid_sys_read_hex_string_value(&stored_report->payload[8], 2, error_flag);
					
					
					local_received = grid_sys_read_hex_string_value(&message[8], 2, error_flag);
					remote_received = grid_sys_read_hex_string_value(&message[6], 2, error_flag);
										
					
					if (por->partner_status == 0){
						
						if (por->direction == GRID_CONST_NORTH){
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_NORTH);
						}else if (por->direction == GRID_CONST_EAST){
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_EAST);
						}else if (por->direction == GRID_CONST_SOUTH){
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_SOUTH);
						}else if (por->direction == GRID_CONST_WEST){
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_WEST);
						}
						
						
						if (local_stored == 255){ // I have no clue				
							
							// Generate new local		
								
							uint8_t new_local = grid_sys_rtc_get_time(&grid_sys_state)%128;
							local_stored = new_local;
							grid_sys_write_hex_string_value(&stored_report->payload[6], 2, new_local);
						
							grid_msg_checksum_write(stored_report->payload, stored_report->payload_length, grid_msg_checksum_calculate(stored_report->payload, stored_report->payload_length));
							
							// No chance to connect now
							
																			
						}
						else if (remote_received == 255){
												
							// Remote is clueless
							// No chance to connect now
							
						}
						if (remote_received != remote_stored){
							
							
							grid_sys_write_hex_string_value(&stored_report->payload[8], 2, remote_received);
							
							remote_stored = remote_received;

							grid_msg_checksum_write(stored_report->payload, stored_report->payload_length, grid_msg_checksum_calculate(stored_report->payload, stored_report->payload_length));
														
							// Store remote								
							// No chance to connect now
						}
						if (local_stored != local_received){
									
							// Remote is clueless
							// No chance to connect now
							
						}
						else{
																			
							// CONNECT
							por->partner_fi = (message[3] - por->direction + 6)%4;
							por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
							por->partner_status = 1;
							
							grid_sys_state.age = grid_sys_rtc_get_time(&grid_sys_state);
							
								
							GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Connect");
							
							grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN
							

							
						}
						
						
													
													
					}
					else{
													
						// VALIDATE CONNECTION
						uint8_t validator = 1;
						
						validator &= local_received == local_stored;
						validator &= remote_received == remote_stored;	
												
						validator &= por->partner_fi == (message[3] - por->direction + 6)%4;
						validator &= por->partner_hwcfg == grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
						
													
						if (validator == 1){
							
							// OK nice job!
							
							//printf("LS: %d RS: %d LR: %d RR: %d  (Validate)\r\n",local_stored,remote_stored,local_received,remote_received);
														
							//OK
							//grid_sys_alert_set_alert(&grid_sys_state, 6, 6, 6, 0, 200); // LIGHT WHITE							
														
						}
						else{
															
							//FAILED, DISCONNECT
							por->partner_status = 0;
							
							grid_sys_write_hex_string_value(&stored_report->payload[8], 2, 255);
							grid_sys_write_hex_string_value(&stored_report->payload[6], 2, 255);
							grid_msg_checksum_write(stored_report->payload, stored_report->payload_length, grid_msg_checksum_calculate(stored_report->payload, stored_report->payload_length));														
							
							//printf("LS: %d RS: %d LR: %d RR: %d  (Invalid)\r\n",local_stored,remote_stored,local_received,remote_received);										
							
							grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 255, 2, 200); // Purple
														
								
						}
					}
								
								
				}
																		
			}
			else{ // Unknown Message Type
					
				grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200); // RED SHORT
				printf("{\"type\": \"WARNING\", \"data\": [\"Unknow Message Type\"]}\r\n");
							
			}
						

						
		}
		else{
			// INVALID CHECKSUM
			
			printf("{\"type\": \"WARNING\", \"data\": [\"Invalid Checksum\"]}\r\n");
	
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
		

		printf("{\"type\": \"ERROR\", \"data\": [\"Frame Error\"]}\r\n");
	}
			
	return;
	
}

void grid_port_receive_complete_task(struct grid_port* por){
	
	if (por->usart_error_flag == 1){
		
		por->usart_error_flag = 0;
		
		grid_port_reset_receiver(por);	
				
		grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 0, 500); // White triangle
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
			
		if (mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] == 0){
				
			mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] = 1;
				
		}
		else{

				
			mod->report_array[GRID_REPORT_INDEX_MAPMODE].helper[0] = 0;
				
			uint8_t current_bank = grid_sys_get_bank(&grid_sys_state);
			uint8_t new_bank = (current_bank + 1)%2;
						
			grid_report_sys_set_payload_parameter(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE,GRID_CLASS_BANKACTIVE_BANKNUMBER_offset,GRID_CLASS_BANKACTIVE_BANKNUMBER_length, new_bank);
		
			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE);
			
			

			 
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

#define RTC1MS (RTC1SEC/1000)

void init_timer(void)
{
	
		
	//RTC_Scheduler_ping.interval = RTC1SEC/20; //50ms
	RTC_Scheduler_ping.interval = RTC1MS*GRID_PARAMETER_PING_INTERVAL;
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_heartbeat.interval = RTC1MS*GRID_PARAMETER_HEARTBEAT_INTERVAL;
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

//====================== USB TEST =====================//

int main(void)
{
	
	

	atmel_start_init();	
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "Start Initialized");

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "D51 Init");
	grid_d51_init(); // Check User Row


	#include "usb/class/midi/device/audiodf_midi.h"
	audiodf_midi_init();

	composite_device_start();

	uint8_t usb_testbuffer_ptr = 0;
	uint8_t* usb_testbuffer;
	grid_usb_serial_init(usb_testbuffer);

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Initialized");
		
	grid_module_common_init();
		
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Grid Module Initialized");

	init_timer();
	
	uint32_t loopcounter = 0;
	
	uint32_t loopstart = 0;
	
	uint32_t loopslow = 0;
	uint32_t loopfast = 0;
	uint32_t loopwarp = 0;
	
	uint8_t usb_init_variable = 0;
	
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Entering Main Loop");
	

	
	while (1) {
		
				
		grid_task_enter_task(&grid_task_state, GRID_TASK_UNDEFINED);
		
		
// 		if (loopcounter%5 == 0){
// 			
// 			if (GRID_PORT_N.partner_status == 0){
// 				grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_NORTH);
// 			}
// 			
// 			if (GRID_PORT_E.partner_status == 0){
// 				grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_EAST);
// 			}
// 			
// 			if (GRID_PORT_S.partner_status == 0){
// 				grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_SOUTH);
// 			}
// 			
// 			if (GRID_PORT_W.partner_status == 0){
// 				grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_PING_WEST);
// 			}
// 			
// 		}
				
		
		if (usb_init_variable == 0){
			
	
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{		
				
				grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN		
				GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Connected");
				
				uint8_t new_bank = 0;
				
				grid_report_sys_set_payload_parameter(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE,GRID_CLASS_BANKACTIVE_BANKNUMBER_offset,GRID_CLASS_BANKACTIVE_BANKNUMBER_length, new_bank);
	
				grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE);
				
				usb_init_variable = 1;
			}
			
		}
		
		
		
		// Request neighbour bank settings if we don't have it initialized
		
 		if (grid_sys_get_bank(&grid_sys_state) == 255){
 										
 			grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_CFG_REQUEST);
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
			
			printf("{\"type\":\"LOOP\", \"data\": [\"%d\", \"%d\", \"%d\", \"%d\"]}\r\n", loopcounter, loopslow, loopfast, loopwarp);
		
			loopcounter = 0;
			loopslow = 0;
			loopfast = 0;
			loopwarp = 0;
		}
		
		
		
							
		grid_task_enter_task(&grid_task_state, GRID_TASK_RECEIVE);

		// CHECK RX BUFFERS
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);
			
	
		cdcdf_acm_read(GRID_PORT_H.rx_double_buffer, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);			
		
		uint8_t usblength = strlen(usb_testbuffer);
		
		if (usblength){	
						
			GRID_PORT_H.rx_double_buffer_read_start_index = 0;

			grid_port_receive_decode(&GRID_PORT_H, 0, usblength-2);
								
			for (uint8_t i = 0; i<100; i++){
				
				GRID_PORT_H.rx_double_buffer[i] = 0;
			
			}
				
		}
				
					
	
	
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
		
		grid_port_process_inbound(&GRID_PORT_H, 0);				
		
		
		
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

					grid_led_set_min(&grid_led_state, i, GRID_LED_LAYER_ALERT, color_r*0   , color_g*0   , color_b*0);
					grid_led_set_mid(&grid_led_state, i, GRID_LED_LAYER_ALERT, color_r*0.5 , color_g*0.5 , color_b*0.5);
					grid_led_set_max(&grid_led_state, i, GRID_LED_LAYER_ALERT, color_r*1   , color_g*1   , color_b*1);
						
					
				}
		
			}
			
			uint8_t intensity = grid_sys_alert_get_color_intensity(&grid_sys_state);
	
			for (uint8_t i=0; i<grid_led_state.led_number; i++){	
				//grid_led_set_color(i, 0, 255, 0);	
		
				grid_led_set_phase(&grid_led_state, i, GRID_LED_LAYER_ALERT, intensity);
								
			}
			
			
		}
		
		grid_task_enter_task(&grid_task_state, GRID_TASK_LED);
	
		grid_led_tick(&grid_led_state);
	

		
		if (loopcounter%1 == 0){
			
			grid_led_render_all(&grid_led_state);	
			
						
// 	 		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
// 	
// 	 		}
			grid_led_hardware_start_transfer(&grid_led_state);
		}		
		
		
	
	
	
	
		grid_task_enter_task(&grid_task_state, GRID_TASK_IDLE);


		// IDLETASK
		
		
		uint32_t elapsed = grid_sys_rtc_get_elapsed_time(&grid_sys_state, loopstart);
		
		if (elapsed < RTC1MS){
			
			if (loopwarp>5){
				
				if (RTC1MS - elapsed > 0){
					
					if ((RTC1MS - elapsed)<loopwarp){				
						loopwarp-=(RTC1MS - elapsed);
						loopstart-=(RTC1MS - elapsed);
					}
					else{
						loopwarp-=loopwarp;
						loopstart-=loopwarp;
					}
					
					loopfast++;
				}
			}
			
			while(grid_sys_rtc_get_elapsed_time(&grid_sys_state, loopstart) < RTC1SEC/1000){	
					
				delay_us(1);			
			}	
					
		}
		else{
			loopwarp+= elapsed - RTC1MS;
			
			loopslow++;
		}
		
		grid_task_enter_task(&grid_task_state, GRID_TASK_UNDEFINED);		

		

	}//WHILE
	
	
	
}//MAIN

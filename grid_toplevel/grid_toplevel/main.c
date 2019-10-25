


#include <atmel_start.h>
#include "atmel_start_pins.h"



#include <stdio.h>

#include "../../grid_lib/grid_protocol.h"

#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_sys.c" // Grid System


#include "../../grid_lib/grid_buf.c" // Grid Buffer Abstraction


#include "../../grid_modules/grid_module.c" // 


static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;


volatile uint8_t pingflag = 0;

volatile uint32_t realtime = 0; 


void grid_port_receive_task(GRID_PORT_t* por){
		

	// THERE IS ALREADY DATA, PROCESS THAT FIRST
	if	(por->rx_double_buffer_status == 1){
		return;
	}
	
	

		
	if (por->rx_double_buffer_timeout > 20000){
		
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
		
		

	
	
			

	

	for(uint16_t i = 0; i<10; i++){
		
		if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] != '\n' && por->rx_double_buffer[por->rx_double_buffer_seek_start_index] != 0)
		{
			por->rx_double_buffer_seek_start_index++;			
			por->rx_double_buffer_seek_start_index%=GRID_DOUBLE_BUFFER_RX_SIZE;
		}	
		else{			
			i = GRID_DOUBLE_BUFFER_RX_SIZE;
		}
		
// 		if (i == 9){
// 			por->rx_double_buffer_seek_start_index = 0;
//			por->rx_double_buffer_read_start_index = 0;
//			grid_sys_port_reset_dma(por);
// 		}
	}
		
					

	if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == '\n'){
		por->rx_double_buffer_timeout = 0;
		por->rx_double_buffer_status = 1;
	}
	
	if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 0){

	}
		
}

void grid_port_receive_decode(GRID_PORT_t* por, uint8_t startcommand, uint8_t length){
	
	
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
				
	for (uint8_t i = 0; i<length; i++){
		message[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
	}
				
	// IMPLEMENT CHECKSUM VALIDATOR HERE
	if (length>5){
					
		checksum_received = grid_sys_read_hex_string_value(&message[length-3], 2, &error_flag);
					
		checksum_calculated = grid_msg_get_checksum(message, length);
					
		// IF CHECKSUM IS VALID
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
				
				
							
				uint8_t updated_age = received_age + 1;
								
				
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
					
			}
			else if (message[1] == GRID_MSG_DIRECT){ // Direct Message
												
				//process direct message
							
				if (message[2] == GRID_MSG_ACKNOWLEDGE){				

					grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 255, 2, 200); // PURPLE
				}
				else if (message[2] == GRID_MSG_NACKNOWLEDGE){
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
						
						grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 2, 200); // GREEN
															
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
							grid_sys_alert_set_alert(&grid_sys_state, 0, 0, 10, 2, 200); // BLUE
							
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
						
			grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 255, 1, 2000); // RED BLINKY
						
		}
				

	}
	else{

	}
				
	if (message[1] == GRID_MSG_BROADCAST){				
		// RESPOND WITH ACK OR NACK
		
		uint8_t response_length = strlen(response);
		
		if(grid_buffer_write_init(&por->tx_buffer, response_length)){
						
			
			uint8_t checksum[4];
			
			sprintf(checksum, "%02x", grid_msg_get_checksum(response, response_length));
			
			response[4] = checksum[0];
			response[5] = checksum[1];

						
			for (uint8_t i=0; i<response_length; i++)
			{
				grid_buffer_write_character(&por->tx_buffer, response[i]);
			}
						
			grid_buffer_write_acknowledge(&por->tx_buffer);
						
		}
					
	}
	
	for (uint8_t i = 0; i<length; i++){
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
	}
		
	por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length)%GRID_DOUBLE_BUFFER_RX_SIZE;
	por->rx_double_buffer_seek_start_index =  por->rx_double_buffer_read_start_index;
	
	
	por->rx_double_buffer_status = 0;


	

	return;
	
}

void grid_port_receive_complete_task(GRID_PORT_t* por){
	
	if (por->rx_double_buffer_status != 1){
		return;
	}

	
	uint8_t length = 0;
	
	if (por->rx_double_buffer_read_start_index < por->rx_double_buffer_seek_start_index){
		length = por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	else{
		length = GRID_DOUBLE_BUFFER_RX_SIZE + por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	
		
	grid_port_receive_decode(por, por->rx_double_buffer_read_start_index, length);
			

	
	por->rx_double_buffer_status = 0;
	
					
	//grid_port_process_outbound_usart(por);
	
	
}


static void RTC_Scheduler_rx_task_cb(const struct timer_task *const timer_task)
{

	grid_port_receive_task(&GRID_PORT_N);
	grid_port_receive_task(&GRID_PORT_E);
	grid_port_receive_task(&GRID_PORT_S);
	grid_port_receive_task(&GRID_PORT_W);
	
	
}

static void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{

	realtime++;
	
	
}

static void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
{
	pingflag = 1;
}

#define RTC1SEC 16384

void init_timer(void)
{
	
		
	RTC_Scheduler_ping.interval = 16380/5; //1sec
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_rx_task.interval = RTC1SEC/10000; // 100us
	RTC_Scheduler_rx_task.cb       = RTC_Scheduler_rx_task_cb;
	RTC_Scheduler_rx_task.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_realtime.interval = 1; // 1us
	RTC_Scheduler_realtime.cb       = RTC_Scheduler_realtime_cb;
	RTC_Scheduler_realtime.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_rx_task);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
	
	timer_start(&RTC_Scheduler);
	
}

struct io_descriptor *io;





int main(void)
{
	
			

	
	
	#include "usb/class/midi/device/audiodf_midi.h"
	

	
	atmel_start_init();	
	
	//TIMER_0_example2();

	audiodf_midi_init();



	composite_device_start();
	
	grid_module_init();
	



	
	init_timer();	
	
	uint32_t loopstart = 0;
		
	while (1) {
					
		loopstart = realtime;
		

		
		
		
		
		/* ========================= PING ============================= */
						
		if (pingflag){
			
			grid_sys_ping(&GRID_PORT_N);
			grid_sys_ping(&GRID_PORT_E);
			grid_sys_ping(&GRID_PORT_S);
			grid_sys_ping(&GRID_PORT_W);
			
			pingflag = 0;
		}
		
		
		// CHECK RX BUFFERS
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);
		
				
		/* ========================= UI_PROCESS_INBOUND ============================= */
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
				
				
			
		uint16_t length = 0;
		
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
	
			for (uint8_t i=0; i<16; i++){	
				//grid_led_set_color(i, 0, 255, 0);	
		
				grid_led_set_phase(&grid_led_state, i, 1, intensity);
								
			}
			
			
		}
		
				
		grid_led_tick(&grid_led_state);
		grid_led_render_all(&grid_led_state);
				
				
		grid_led_hardware_start_transfer_blocking(&grid_led_state);
	
			
		
		
		
		// IDLETASK
		while(loopstart + RTC1SEC/1000 > realtime){
			delay_us(10);
		}	
		
		
	}
}

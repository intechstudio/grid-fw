#include <atmel_start.h>
#include "atmel_start_pins.h"



#include <stdio.h>

#include "../../grid_lib/grid_protocol.h"

#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry
#include "../../grid_lib/grid_sys.c" // Grid System


#include "../../grid_lib/grid_buf.c" // Grid Buffer Abstraction

#define GRID_MODULE_P16


#include "../../grid_modules/grid_module_p16.c" // 


volatile uint8_t task1flag = 0;

volatile uint8_t reportflag = 0;







static struct timer_task RTC_Scheduler_tick;
static struct timer_task RTC_Scheduler_report;
static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;



#define TASK_UNDEFINED 0
#define TASK_IDLE	   1
#define TASK_LED	2
#define TASK_UIIN    3
#define TASK_GRID   4

volatile uint8_t task_current = 0;
volatile uint32_t task_counter[8] = {0, 0, 0, 0, 0, 0, 0, 0 };

volatile uint8_t pingflag = 0;

volatile uint32_t realtime = 0; 

static void RTC_Scheduler_tick_cb(const struct timer_task *const timer_task)
{
	realtime++;
	task_counter[task_current]++;
}

 static void RTC_Scheduler_report_cb(const struct timer_task *const timer_task)
 {
 if (reportflag<255) reportflag++;
 }






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
			
			grid_sys_error_set_alert(&grid_sys_state, 255, 255, 255, 2, 200);
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
					
		checksum_calculated = grid_sys_calculate_checksum(message, length-3);
					
		// IF CHECKSUM IS VALID
		if (checksum_calculated == checksum_received && error_flag == 0){
						
			if (message[1] == GRID_MSG_BROADCAST){ // Broadcast message
							
				// IF WE CAN STORE THE MESSAGE IN THE RX BUFFER
				if (grid_buffer_write_init(&por->rx_buffer, length)){
					for (uint8_t i=0; i<length; i++){
									
						grid_buffer_write_character(&por->rx_buffer, message[i]);
									
					}
								
					grid_buffer_write_acknowledge(&por->rx_buffer);
								
					grid_port_process_inbound(por);
																
					response[2] = GRID_MSG_ACKNOWLEDGE;
				}
							
			}
			else if (message[1] == GRID_MSG_DIRECT){ // Direct Message
							
							
				//process direct message
							
				if (message[2] == GRID_MSG_ACKNOWLEDGE){
					
// 					grid_sys_state.error_code = 5; // Purple
// 					grid_sys_state.error_style = 2; // CONST
// 					grid_sys_state.error_state = 200; // CONST
					grid_sys_error_set_alert(&grid_sys_state, 255, 0, 255, 2, 200);
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
									
// 						grid_sys_state.error_code = 2; // GREEN
// 						grid_sys_state.error_style = 2; // CONST
// 						grid_sys_state.error_state = 200; // CONST
						
						grid_sys_error_set_alert(&grid_sys_state, 0, 255, 0, 2, 200);
						
								
									
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
							
// 							grid_sys_state.error_code = 7; // WHITE
// 							grid_sys_state.error_style = 2; // CONST
// 							grid_sys_state.error_state = 200; // CONST
													
							grid_sys_error_set_alert(&grid_sys_state, 255, 255, 255, 2, 200);
										
						}
						else{
							//OK
							
// 							grid_sys_state.error_code = 1; // BLUE
// 							grid_sys_state.error_style = 2; // CONST
// 							grid_sys_state.error_state = 200; // CONST

							grid_sys_error_set_alert(&grid_sys_state, 0, 0, 10, 2, 200);
							
						}
									
									
					}
								
								
				}
							
							
							
							
							
							
			}
			else{ // Invalid
							
// 				grid_sys_state.error_code = 4; // RED
// 				grid_sys_state.error_style = 2; // CONST
// 				grid_sys_state.error_state = 200; // CONST						
				
				grid_sys_error_set_alert(&grid_sys_state, 255, 0, 0, 2, 200);
							
			}
						

						
		}
		else{
			// INVALID CHECKSUM

// 			grid_sys_state.error_state = 2000;
// 			grid_sys_state.error_style = 1;
// 			grid_sys_state.error_code = 4; //RED
			
			grid_sys_error_set_alert(&grid_sys_state, 255, 0, 255, 1, 2000);
						
		}
				

	}
	else{

	}
				
	if (message[1] == GRID_MSG_BROADCAST){
					
		// RESPOND WITH ACK OR NACK
		
		uint8_t response_length = strlen(response);
		
		if(grid_buffer_write_init(&por->tx_buffer, response_length)){
						
			
			uint8_t checksum[4];
			
			sprintf(checksum, "%02x", grid_sys_calculate_checksum(response, response_length-3));
			
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

static void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
{
	pingflag = 1;
}

#define RTC1SEC 16384

void init_timer(void)
{
	
	
	
	
	RTC_Scheduler_tick.interval = 1;
	RTC_Scheduler_tick.cb       = RTC_Scheduler_tick_cb;
	RTC_Scheduler_tick.mode     = TIMER_TASK_REPEAT;
	
	
	RTC_Scheduler_report.interval = 32768/2; //1sec
	RTC_Scheduler_report.cb       = RTC_Scheduler_report_cb;
	RTC_Scheduler_report.mode     = TIMER_TASK_REPEAT;
	
		
	RTC_Scheduler_ping.interval = 16380/5; //1sec
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_rx_task.interval = RTC1SEC/10000; // 100us
	RTC_Scheduler_rx_task.cb       = RTC_Scheduler_rx_task_cb;
	RTC_Scheduler_rx_task.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_tick);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_rx_task);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_start(&RTC_Scheduler);
	
	

}







int main(void)
{
	
			

	
	
	#include "usb/class/midi/device/audiodf_midi.h"
	
	atmel_start_init();
	
	
	//TIMER_0_example2();

	audiodf_midi_init();



	composite_device_start();
	
	grid_module_init();
	


	for (uint8_t i = 0; i<255; i++){
		
		// SEND DATA TO LEDs
		
					
		uint8_t color_r   = i;
		uint8_t color_g   = i;
		uint8_t color_b   = i;
					
					
		for (uint8_t i=0; i<16; i++){
			//grid_led_set_color(i, 0, 255, 0);
			grid_led_set_color(i, color_r, color_g, color_b);
						
		}
		
		
		dma_spi_done = 0;
		spi_m_dma_enable(&GRID_LED);
	
		io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
	
		while (dma_spi_done == 0)
		{
			
		}
		
		delay_ms(1);
						
	}
	
	init_timer();



	uint32_t faketimer = 0;

	uint16_t colorfade = 0;
	uint8_t colorcode = 0;

	uint8_t mapmode = 1;
	uint8_t sysmode = 1;
	
	
	uint32_t loopcounter = 0;
	
	
	uint32_t loopstart = 0;
	
	char system_report_tasks[200];
	char system_report_buffers[200];
	char system_report_grid[200];
	

	while (1) {
		
		
		// CHECK RX BUFFERS
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);
		
				
				
		if (pingflag){
			
			grid_sys_ping_all();
			pingflag = 0;
		}
		
		
		// REPORT OVER CDC SERIAL FOR DEBUGING
		if (loopcounter == 100){
			
			
			cdcdf_acm_write(system_report_tasks, strlen(system_report_tasks));		
		}else if (loopcounter == 200){
			cdcdf_acm_write(system_report_grid, strlen(system_report_grid));		
		}else if (loopcounter == 300){
	
		}else if (loopcounter == 400){
	
		}else if (loopcounter == 500){
	
		}
	
		//checktimer flags
		if (reportflag){
			
			sprintf(system_report_tasks, "LOOPTICK %02x\nREALTIME %02x\nTASK0 %02x\nTASK1 %02x\nTASK2 %02x\nTASK3 %02x\nTASK4 %02x\n\0", loopcounter, realtime, task_counter[0], task_counter[1], task_counter[2], task_counter[3], task_counter[4]);

			sprintf(system_report_grid, "N_RX_C %02x\nE_RX_C %02x\nS_RX_C %02x\nW_RX_C %02x\nN_TX_C %02x\nE_TX_C %02x\nS_TX_C %02x\nW_TX_C %02x\nN_BELL_C %02x\nE_BELL_C %02x\nS_BELL_C %02x\nW_BELL_C %02x\n\0", 
				grid_sys_rx_counter[GRID_SYS_NORTH],	
				grid_sys_rx_counter[GRID_SYS_EAST],	
				grid_sys_rx_counter[GRID_SYS_SOUTH], 
				grid_sys_rx_counter[GRID_SYS_WEST], 
				grid_sys_tx_counter[GRID_SYS_NORTH], 
				grid_sys_tx_counter[GRID_SYS_EAST], 
				grid_sys_tx_counter[GRID_SYS_SOUTH], 
				grid_sys_tx_counter[GRID_SYS_WEST],
				grid_sys_ping_counter[GRID_SYS_NORTH],
				grid_sys_ping_counter[GRID_SYS_EAST],
				grid_sys_ping_counter[GRID_SYS_SOUTH],
				grid_sys_ping_counter[GRID_SYS_WEST]
			);

			
			grid_sys_rx_counter[GRID_SYS_NORTH]=0;
			grid_sys_rx_counter[GRID_SYS_EAST]=0;
			grid_sys_rx_counter[GRID_SYS_SOUTH]=0;
			grid_sys_rx_counter[GRID_SYS_WEST]=0;
			
			grid_sys_tx_counter[GRID_SYS_NORTH]=0;
			grid_sys_tx_counter[GRID_SYS_EAST]=0;
			grid_sys_tx_counter[GRID_SYS_SOUTH]=0;
			grid_sys_tx_counter[GRID_SYS_WEST]=0;
			
			grid_sys_ping_counter[GRID_SYS_NORTH]=0;
			grid_sys_ping_counter[GRID_SYS_EAST]=0;
			grid_sys_ping_counter[GRID_SYS_SOUTH]=0;
			grid_sys_ping_counter[GRID_SYS_WEST]=0;
			
			
			
			realtime = 0;
			loopcounter = 0;
			reportflag--;
			
			for (uint8_t i=0; i<8; i++)
			{
				task_counter[i] = 0;
			}
		}
		
		loopcounter++;
			
			
		loopstart = realtime;
		
		
		if (mapmode != gpio_get_pin_level(MAP_MODE)){
			
			//hiddf_mouse_move(-5, HID_MOUSE_X_AXIS_MV);
		
			if (mapmode == 1){
				
								
				static struct hiddf_kb_key_descriptors key_array[]      = {
					{HID_CAPS_LOCK, false, HID_KB_KEY_DOWN},
				};
				
				//hiddf_keyboard_keys_state_change(key_array, 1);
				//hiddf_mouse_move(-20, HID_MOUSE_X_AXIS_MV);
				
				
				audiodf_midi_xfer_packet(0x09, 0x90, 0x64, 0x64); // cable 0 channel 0 note on 
								
			}
			else{
								
				static struct hiddf_kb_key_descriptors key_array[]      = {
					{HID_CAPS_LOCK, false, HID_KB_KEY_UP},
				};
				
				
				//hiddf_keyboard_keys_state_change(key_array, 1);
				audiodf_midi_xfer_packet(0x08, 0x80, 0x64, 0x0); // cable 0 channel 0 note off
			
			}
			
			
			

			
			if (mapmode==0){

				
				uint8_t r[4] = {255*(random()%2), 255*(random()%2), 255*(random()%2), 255*(random()%2)};
				uint8_t g[4] = {255*(random()%2), 255*(random()%2), 255*(random()%2), 255*(random()%2)};
				uint8_t b[4] = {255*(random()%2), 255*(random()%2), 255*(random()%2), 255*(random()%2)};
				
				for (uint8_t i = 0; i<4; i++){
					
					if (r[i] == 0 && g[i] == 0 && b[i]==0){
						
						uint8_t ra = random()%3;
						
						if (ra == 0){
							r[i] = 255;
						}
						else if (ra == 1){
							g[i] = 255;
						}
						else{
							b[i] = 255;
						}
						
						
						
					}
					
				}
				
				
				
				for (uint8_t i = 0; i<16; i++){
					
					grid_led_set_max(i, 0, r[i%4], g[i%4], b[i%4]);
					grid_led_set_mid(i, 0, r[i%4]/2, g[i%4]/2, b[i%4]/2);
					
				}
				
				
				
				sysmode = ! sysmode;
				
				if (sysmode){					
					grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_MASTER);
					
				}
				else{
					grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_SLAVE);
				}
				
				
			}
			
			mapmode = !mapmode;
					
		}
		
		
		
		if (faketimer > 100){
			grid_tel_frequency_tick();
			faketimer = 0;
		}
		faketimer++;

			
		// SYNC TEST
		if (sysmode){
			
			grid_sync_set_level(GRID_SYNC_1, loopcounter%2);
			
		}
		
		
		
			
		/* ========================= UI_PROCESS_INBOUND ============================= */
		
		// Push out all changes
		
		task_current = TASK_UIIN;
		
		task_current = TASK_UNDEFINED;

			
		
		/* ========================= GRID MOVE TASK ============================= */		
		
		uint16_t length = 0;
		
		grid_port_process_ui(&GRID_PORT_U);		
		
		grid_port_process_inbound(&GRID_PORT_U); // Copy data from UI_RX to HOST_TX & north TX AND STUFF

		grid_port_process_inbound(&GRID_PORT_N);		
		grid_port_process_inbound(&GRID_PORT_E);		
		grid_port_process_inbound(&GRID_PORT_S);		
		grid_port_process_inbound(&GRID_PORT_W);		
		
		grid_port_process_outbound_usart(&GRID_PORT_N);
		grid_port_process_outbound_usart(&GRID_PORT_E);
		grid_port_process_outbound_usart(&GRID_PORT_S);
		grid_port_process_outbound_usart(&GRID_PORT_W);
		
		grid_port_process_outbound_usb(&GRID_PORT_H); // Send data from HOST_TX through USB
		grid_port_process_outbound_ui(&GRID_PORT_U);
		
	

				
				
				
				
				
				
		if (sysmode == 1){
			
			task_current = TASK_LED;
			
			grid_led_tick();		
			//RENDER ALL OF THE LEDs
			grid_led_render_all();
			
			task_current = TASK_UNDEFINED;
			
		}
		
		if (sysmode == 0){
			
			for (uint8_t i=0; i<16; i++){
				
				//grid_led_set_color(i, 0, 255, 0);
				
				grid_led_set_color(i, colorfade*(colorcode==0)/4, colorfade*(colorcode==1)/4, colorfade*(colorcode==2)/4);
				
				
			}
			
			colorfade++;
			
			if (colorfade == 4*256){
				colorfade = 0;
			}
			
			if (colorfade == 0) colorcode++;
			if (colorcode>2) colorcode=0;	
			
		}
		
		
		
		if (grid_sys_state.error_state){
			
			grid_sys_state.error_state--;


			uint8_t intensity = grid_sys_error_intensity(&grid_sys_state);
			
			uint8_t color_r   = grid_sys_error_get_color_r(&grid_sys_state) * (intensity/256.0);
			uint8_t color_g   = grid_sys_error_get_color_g(&grid_sys_state) * (intensity/256.0);
			uint8_t color_b   = grid_sys_error_get_color_b(&grid_sys_state) * (intensity/256.0);
			
			
			for (uint8_t i=0; i<16; i++){	
				//grid_led_set_color(i, 0, 255, 0);		
				grid_led_set_color(i, color_r, color_g, color_b);
					
			}
			
			
		}
		
		
		
		
		
		// SEND DATA TO LEDs
		dma_spi_done = 0;
		spi_m_dma_enable(&GRID_LED);
			
		io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
			
		while (dma_spi_done == 0)
		{
		}	
		
		
		
		// IDLETASK
		task_current = TASK_IDLE;
		while(loopstart + RTC1SEC/1000 > realtime){
			delay_us(10);
		}
		
		task_current = TASK_UNDEFINED;
		
		
		
		
	}
}

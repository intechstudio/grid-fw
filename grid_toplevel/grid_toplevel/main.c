

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



static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_report;
static struct timer_task RTC_Scheduler_heartbeat;

static void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
{

	pingflag++;
	
	switch (pingflag%4)
	{
		case 0:
			GRID_PORT_N.ping_flag = 1;
			break;
		case 1:
			GRID_PORT_E.ping_flag = 1;
			break;
		case 2:
			GRID_PORT_S.ping_flag = 1;
			break;
		case 3:
			GRID_PORT_W.ping_flag = 1;
			break;
	}
	
}


static void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{
	grid_sys_rtc_tick_time(&grid_sys_state);	
	grid_task_timer_tick(&grid_task_state);
			
	uint8_t mapmode_value = !gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != grid_sys_state.mapmodestate){
		
		grid_sys_state.mapmodestate = mapmode_value;
			
		if (grid_sys_state.mapmodestate == 0){ // RELEASE
			
			grid_debug_print_text("MAPMODE_RELEASE");
			
			uint8_t event_index = grid_ui_event_find(&grid_core_state.element[0], GRID_UI_EVENT_MAPMODE_RELEASE);
			grid_ui_event_template_action(&grid_core_state.element[0], event_index);
			grid_ui_event_trigger(&grid_core_state.element[0].event_list[event_index]);		
								
		}
		else{ // PRESS
		
		

			grid_debug_print_text("MAPMODE_PRESS");
			
			uint8_t event_index = grid_ui_event_find(&grid_core_state.element[0], GRID_UI_EVENT_MAPMODE_PRESS);
			grid_ui_event_template_action(&grid_core_state.element[0], event_index);
			grid_ui_event_trigger(&grid_core_state.element[0].event_list[event_index]);		
						
									 
		}

	}

}

static void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{

	uint8_t event_index = grid_ui_event_find(&grid_core_state.element[0], GRID_UI_EVENT_HEARTBEAT);				
	grid_ui_event_template_action(&grid_core_state.element[0], event_index);	
	grid_ui_event_trigger(&grid_core_state.element[0].event_list[event_index]);

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
	RTC_Scheduler_ping.interval = RTC1MS*GRID_PARAMETER_PING_interval;
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_heartbeat.interval = RTC1MS*GRID_PARAMETER_HEARTBEAT_interval;
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


	grid_usb_serial_init();
	grid_usb_midi_init();
		

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
	
	
	uint8_t debug_flag = 0;	
	uint16_t debug_offset = 0;
	uint8_t debug[500] = {0};


	grid_sys_load_bank_settings(&grid_sys_state, &grid_nvm_state);
	
	while (1) {
	
				
		grid_task_enter_task(&grid_task_state, GRID_TASK_UNDEFINED);
		
		
		
		if (usb_init_variable == 0){
			
	
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{			
			
				grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN	
				
				GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Connected");
				
				grid_sys_set_bank(&grid_sys_state, grid_sys_get_bank_next(&grid_sys_state));
				
				uint8_t event_index = grid_ui_event_find(&grid_core_state.element[0], GRID_UI_EVENT_CFG_RESPONSE);
				grid_ui_event_template_action(&grid_core_state.element[0], event_index);
				grid_ui_event_trigger(&grid_core_state.element[0].event_list[event_index]);
				
				usb_init_variable = 1;
				
			}
			
		}
		
		
		
		// Request neighbor bank settings if we don't have it initialized
		
 		if (grid_sys_get_bank_num(&grid_sys_state) == 255 && loopcounter%80 == 0){
 										
 			uint8_t event_index = grid_ui_event_find(&grid_core_state.element[0], GRID_UI_EVENT_CFG_REQUEST);
 			grid_ui_event_template_action(&grid_core_state.element[0], event_index);
 			grid_ui_event_trigger(&grid_core_state.element[0].event_list[event_index]);
			 
 		}

		
		loopcounter++;
	
		loopstart = grid_sys_rtc_get_time(&grid_sys_state);
		
		if (scheduler_report_flag){
			
			
			
			scheduler_report_flag=0;
		
			uint32_t task_val[GRID_TASK_NUMBER] = {0};
				
			for(uint8_t i = 0; i<GRID_TASK_NUMBER; i++){
	
				task_val[i] = grid_task_timer_read(&grid_task_state, i);
			
			}
			grid_task_timer_reset(&grid_task_state);
			
			loopcounter = 0;
			loopslow = 0;
			loopfast = 0;
			loopwarp = 0;
		}
		
		
		
							
		grid_task_enter_task(&grid_task_state, GRID_TASK_RECEIVE);

		
		// MIDI READ TEST CODE
		
		uint8_t midi_rx_buffer[10] = {0};
		uint8_t midi_rx_length = 0;
		
		audiodf_midi_read(midi_rx_buffer,4);
		
		midi_rx_length = strlen(midi_rx_buffer);		
		
		/*		
		if (midi_rx_buffer[0]!=0 || midi_rx_buffer[1]!=0 || midi_rx_buffer[2]!=0 || midi_rx_buffer[3]!=0){
			grid_sys_alert_set_alert(&grid_sys_state, 50,0,0,2,500); // SOFT RED
			
			uint8_t message[30] = {0};
				
			sprintf(message, "MIDI: %02x %02x %02x %02x\n", midi_rx_buffer[0],midi_rx_buffer[1],midi_rx_buffer[2],midi_rx_buffer[3]);
			
			cdcdf_acm_write(message, strlen(message));
			delay_us(100);
		}	
		*/
		
		
		// SERIAL READ 
	
		cdcdf_acm_read(GRID_PORT_H.rx_double_buffer, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);			
		
		uint16_t usblength = strlen(GRID_PORT_H.rx_double_buffer);
		
		if (usblength){	

						
			GRID_PORT_H.rx_double_buffer_read_start_index = 0;

			grid_port_receive_decode(&GRID_PORT_H, 0, usblength-2);
	
				
		}
				
		// CHECK RX BUFFERS
		
		
		grid_port_receive_complete_task(&GRID_PORT_N);
		grid_port_receive_complete_task(&GRID_PORT_E);
		grid_port_receive_complete_task(&GRID_PORT_S);
		grid_port_receive_complete_task(&GRID_PORT_W);
		
		
		// NVM READ
		if (GRID_PORT_U.rx_double_buffer_status != 0){	
			
			uint32_t length = GRID_PORT_U.rx_double_buffer_status;
							
			if (length !=0){
				
				// GETS HERE
							

				
				grid_port_receive_decode(&GRID_PORT_U, 0, length);		
				
			}	
			else{
				
				//clear buffer
				for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++)
				{
					GRID_PORT_U.rx_double_buffer[i] = 0;
				}
			}
		}
		
							
	
	
		/* ========================= GRID REPORT TASK ============================= */
		grid_task_enter_task(&grid_task_state, GRID_TASK_REPORT);


		grid_port_process_ui(&GRID_PORT_U); // COOLDOWN DELAY IMPLEMENTED INSIDE


		/* ========================= GRID INBOUND TASK ============================= */						
		grid_task_enter_task(&grid_task_state, GRID_TASK_INBOUND);	
		
		// Copy data from UI_RX to HOST_TX & north TX AND STUFF
		grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback
		
		grid_port_process_inbound(&GRID_PORT_N, 0);		
		grid_port_process_inbound(&GRID_PORT_E, 0);		
		grid_port_process_inbound(&GRID_PORT_S, 0);
		grid_port_process_inbound(&GRID_PORT_W, 0);
		
		grid_port_process_inbound(&GRID_PORT_H, 0);	// USB	
		
		
		
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
			
			grid_led_lowlevel_render_all(&grid_led_state);	
			
						
// 	 		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
// 	
// 	 		}
			grid_led_lowlevel_hardware_start_transfer(&grid_led_state);
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

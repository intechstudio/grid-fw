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
volatile uint8_t task2flag = 0;

volatile uint8_t reportflag = 0;




static struct timer_task RTC_Scheduler_tick;
static struct timer_task RTC_Scheduler_report;
static struct timer_task RTC_Scheduler_task2;
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

static void RTC_Scheduler_task2_cb(const struct timer_task *const timer_task)
{
	if (task2flag<255) task2flag++;
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
	
		
	RTC_Scheduler_ping.interval = 16380; //1sec
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_task2.interval = 32768/2*20;
	RTC_Scheduler_task2.cb       = RTC_Scheduler_task2_cb;
	RTC_Scheduler_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_tick);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_task2);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_start(&RTC_Scheduler);
	
	

}

static struct timer_task TIMER_0_task1, TIMER_0_task2;

/**
 * Example of using TIMER_0.
 */
static void TIMER_0_task1_cb2(const struct timer_task *const timer_task)
{
	while (1)
	{
		;
	}
}

static void TIMER_0_task2_cb2(const struct timer_task *const timer_task)
{
}

void TIMER_0_example2(void)
{
	TIMER_0_task1.interval = 100;
	TIMER_0_task1.cb       = TIMER_0_task1_cb2;
	TIMER_0_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_0_task2.interval = 200;
	TIMER_0_task2.cb       = TIMER_0_task2_cb2;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_1, &TIMER_0_task1);
	timer_add_task(&TIMER_1, &TIMER_0_task2);
	timer_start(&TIMER_1);
}



int main(void)
{
	
	#include "usb/class/midi/device/audiodf_midi.h"
	
	atmel_start_init();
	
	
	//TIMER_0_example2();

	audiodf_midi_init();



	composite_device_start();
	
	grid_module_init();
	
	init_timer();
	
	
	
	





	// UI RX EVENT fref=5, alert=50;
	
	struct TEL_event_counter* console_tx = grid_tel_event_register(5, 50);
	
	while(console_tx == NULL){/*TRAP*/}	


	uint32_t faketimer = 0;

	uint8_t colorfade = 0;
	uint8_t colorcode = 0;

	uint8_t mapmode = 1;
	uint8_t sysmode = 0;
	
	
	uint32_t loopcounter = 0;
	
	
	uint32_t loopstart = 0;
	
	char system_report_tasks[200];
	char system_report_buffers[200];
	char system_report_grid[200];
	
	uint8_t current_message_id = 0;

	while (1) {
				
				
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
		
		if (task2flag){
						
			task2flag--;
			
			
		}		
		
		
			
		loopstart = realtime;
		
		
		if (mapmode != gpio_get_pin_level(MAP_MODE)){
			
			//hiddf_mouse_move(-5, HID_MOUSE_X_AXIS_MV);
		
			if (mapmode == 1){
								
				static struct hiddf_kb_key_descriptors key_array[]      = {
					{HID_CAPS_LOCK, false, HID_KB_KEY_DOWN},
				};
				
				hiddf_keyboard_keys_state_change(key_array, 1);
				//audiodf_midi_xfer_packet(0x09, 0x90, 0x64, 0x64); // cable 0 channel 0 note on 
				hiddf_mouse_move(-20, HID_MOUSE_X_AXIS_MV);
								
			}
			else{
								
				static struct hiddf_kb_key_descriptors key_array[]      = {
					{HID_CAPS_LOCK, false, HID_KB_KEY_UP},
				};
				
				
				hiddf_keyboard_keys_state_change(key_array, 1);
				//audiodf_midi_xfer_packet(0x08, 0x80, 0x64, 0x0); // cable 0 channel 0 note off
			
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
				
				
				
			}
			
			mapmode = !mapmode;
					
		}
		
		
		
		if (faketimer > 100){
			grid_tel_frequency_tick();
			faketimer = 0;
		}
		faketimer++;

			
			
		
			
		/* ========================= UI_PROCESS_INBOUND ============================= */
		
		// Push out all changes
		
		task_current = TASK_UIIN;
		
		char txbuffer[256];
		
		
		uint32_t txindex=0;
				
		uint8_t packet_length = 0;
		
		uint8_t len = 0;
		uint8_t id = current_message_id;
		int8_t dx = 0;
		int8_t dy = 0;
				
		uint8_t packetvalid = 0;
				
		sprintf(&txbuffer[txindex],
			"%c%02x%02x%02x%02x",
			GRID_MSG_START_OF_HEADING,
			len, id, dx, dy,
			GRID_MSG_END_OF_BLOCK
		);
		
		txindex += strlen(&txbuffer[txindex]);
		
				
		for (uint8_t i = 0; i<16; i++)
		{
			
			
			if (grid_ain_get_changed(i)){
				
				packetvalid++;
				
				uint16_t average = grid_ain_get_average(i);
				
				
				sprintf(&txbuffer[txindex], "%c%x%02x%02x%02x%02x%c", 
				
					GRID_MSG_START_OF_TEXT, 
					GRID_MSG_PROTOCOL_MIDI, 
					0, // (cable<<4) + channel
					GRID_MSG_COMMAND_MIDI_CONTROLCHANGE,
					i, 
					average/64,
					GRID_MSG_END_OF_TEXT
				);
					

				txindex += strlen(&txbuffer[txindex]);
					
				// UPDATE LEDS (SHOULD USE UI_TX but whatever)
				
				if (grid_sys_get_hwcfg()==64 && i>11){
					grid_led_set_phase(i-4, 0, average*2/128); // 0...255
				}
				else{
					grid_led_set_phase(i, 0, average*2/128); // 0...255	
				}				
				
			}
			
		}
		
		if (packetvalid){
			
			current_message_id++;
						
			// Close the packet
			sprintf(&txbuffer[txindex], "%c", GRID_MSG_END_OF_TRANSMISSION); // CALCULATE AND ADD CRC HERE					
			txindex += strlen(&txbuffer[txindex]);
						
			// Calculate packet length and insert it into the header		
			char length_string[8];
			sprintf(length_string, "%02x", txindex);
			
			txbuffer[1] = length_string[0];
			txbuffer[2] = length_string[1];
			
			
			// Add checksum and linebreak
			sprintf(&txbuffer[txindex], "%02x\n", grid_sys_calculate_checksum(txbuffer, txindex)); // CALCULATE AND ADD CRC HERE
			txindex += strlen(&txbuffer[txindex]);		
			
			// Put the packet into the UI_RX buffer
			if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, txindex)){
				
				for(uint16_t i = 0; i<txindex; i++){
					
					grid_buffer_write_character(&GRID_PORT_U.rx_buffer, txbuffer[i]);
				}
				
				grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);
			}
			
			
		}		
		
		/* ========================= GRID MOVE TASK ============================= */		
		
		uint16_t length = 0;
		
		
		grid_port_process_inbound(&GRID_PORT_U); // Copy data from UI_RX to HOST_TX & north TX AND STUFF

		grid_port_process_inbound_usart(&GRID_PORT_E);		
		
		grid_port_process_outbound_usart(&GRID_PORT_N);
		grid_port_process_outbound_usart(&GRID_PORT_E);
		grid_port_process_outbound_usart(&GRID_PORT_S);
		grid_port_process_outbound_usart(&GRID_PORT_W);
		
		grid_port_process_outbound_usb(&GRID_PORT_H); // Send data from HOST_TX through USB
		grid_port_process_outbound_ui(&GRID_PORT_U);
		
	
		task_current = TASK_UNDEFINED;

				
		if (sysmode == 1){
			
			task_current = TASK_LED;
			
			grid_led_tick();		
			//RENDER ALL OF THE LEDs
			grid_led_render_all();
			
			task_current = TASK_UNDEFINED;
			
			io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
				
			
			while (dma_spi_done == 0)
			{
			}
			
		}
		
		if (sysmode == 0){
			
			for (uint8_t i=0; i<16; i++){
				
				//grid_led_set_color(i, 0, 255, 0);
				
				grid_led_set_color(i, colorfade*(colorcode==0), colorfade*(colorcode==1), colorfade*(colorcode==2));
				
				
			}
			
			colorfade++;
			if (colorfade == 0) colorcode++;
			if (colorcode>2) colorcode=0;
			
			
			delay_ms(2);
			
			// SEND DATA TO LEDs
			dma_spi_done = 0;
			spi_m_dma_enable(&GRID_LED);
			
			io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
			
			while (dma_spi_done == 0)
			{
			}		
			
			
		}
		
		// IDLETASK
		task_current = TASK_IDLE;
		while(loopstart + RTC1SEC/1000 > realtime){
			delay_us(10);
		}
		
		task_current = TASK_UNDEFINED;
	}
}

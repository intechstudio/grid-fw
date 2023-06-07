#include "grid_d51_module.h"


#include <atmel_start.h>
#include "atmel_start_pins.h"

#include <hal_qspi_dma.h>


#include <stdio.h>

#include <string.h>
#include <hpl_reset.h>


#include "grid_msg.h"

#include "usb/class/midi/device/audiodf_midi.h"

volatile uint32_t globaltest = 0;


volatile uint32_t loopcounter = 1;
volatile uint32_t loopcount = 0;

volatile uint8_t midi_rx_buffer[16] = {0};

static void usb_task_inner(){


	grid_keyboard_tx_pop();
	
	// Send midi from Grid to Host!
	grid_midi_tx_pop();        
	


	// MIDI READ TEST CODE
	uint8_t midi_rx_length = 0;

	audiodf_midi_read(midi_rx_buffer,16);


	
	midi_rx_length = strlen(midi_rx_buffer);		
	
	uint8_t found = 0;

	for (uint8_t i=0; i<16; i++){

		if (midi_rx_buffer[i]){
			found++;
		}

	}

		
	if (found){

		//grid_port_debug_printf("MIDI: %02x %02x %02x %02x", midi_rx_buffer[0],midi_rx_buffer[1],midi_rx_buffer[2],midi_rx_buffer[3]);

		uint8_t channel = midi_rx_buffer[1] & 0x0f;
		uint8_t command = midi_rx_buffer[1] & 0xf0;
		uint8_t param1 = midi_rx_buffer[2];
		uint8_t param2 = midi_rx_buffer[3];

		//grid_port_debug_printf("decoded: %d %d %d %d", channel, command, param1, param2);
		
		struct grid_midi_event_desc midi_ev;

		midi_ev.byte0 = channel;
		midi_ev.byte1 = command;
		midi_ev.byte2 = param1;
		midi_ev.byte3 = param2;

		grid_midi_rx_push(midi_ev);


		for (uint8_t i=0; i<16; i++){

			midi_rx_buffer[i] = 0;

		}


	}	


	// Forward midi from Host to Grid!
	grid_midi_rx_pop();
	
	
	// SERIAL READ 

	grid_port_receive_task(&GRID_PORT_H); // USB

}

static void nvm_task_inner(){


	if (grid_ui_bluk_anything_is_in_progress(&grid_ui_state)){
		grid_d51_nvic_set_interrupt_priority_mask(1);
	}
	else{
		if (grid_d51_nvic_get_interrupt_priority_mask() == 1){
			// nvm just entered ready state
			
			// lets reenable ui interrupts
			grid_d51_nvic_set_interrupt_priority_mask(0);
		}
	}

	
	// NVM BULK ERASE
	if (grid_ui_bulk_nvmerase_is_in_progress(&grid_ui_state)){
		
		grid_ui_bulk_nvmerase_next(&grid_ui_state);
	}
	
	// NVM BULK STORE
	if (grid_ui_bulk_pagestore_is_in_progress(&grid_ui_state)){
		
		// START: NEW
		uint32_t cycles_limit = 5000*120;  // 5ms
		uint32_t cycles_start = grid_d51_dwt_cycles_read();

		while(grid_d51_dwt_cycles_read() - cycles_start < cycles_limit){
			grid_ui_bulk_pagestore_next(&grid_ui_state);
		}		
	}
	
	// NVM BULK CLEAR
	if (grid_ui_bulk_pageclear_is_in_progress(&grid_ui_state)){
		
		grid_ui_bulk_pageclear_next(&grid_ui_state);
	}


	// NVM BULK READ
	if (GRID_PORT_U.rx_double_buffer_status == 0){
		
		if (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state)){
			
			

			// START: NEW
			uint32_t cycles_limit = 5000*120;  // 5ms
			uint32_t cycles_start = grid_d51_dwt_cycles_read();

			while(grid_d51_dwt_cycles_read() - cycles_start < cycles_limit){
				grid_ui_bulk_pageread_next(&grid_ui_state);
			}		

			printf("progress\r\n");

			
		}	
	}
	// NVM READ

	uint32_t nvmlength = GRID_PORT_U.rx_double_buffer_status;
						
	if (nvmlength){
			
		GRID_PORT_U.rx_double_buffer_status = 1;
		GRID_PORT_U.rx_double_buffer_read_start_index = 0;
		GRID_PORT_U.rx_double_buffer_seek_start_index = nvmlength-1; //-3
			
		// GETS HERE	
		//grid_port_receive_decode(&GRID_PORT_U, 0, nvmlength-1);		
		grid_port_receive_task(&GRID_PORT_U);	
	}	
		
	//clear buffer
	for (uint32_t i=0; i<GRID_D51_NVM_PAGE_SIZE; i++)
	{
		GRID_PORT_U.rx_double_buffer[i] = 0;
	}

}

static void receive_task_inner(){

	grid_port_receive_task(&GRID_PORT_N);
	grid_port_receive_task(&GRID_PORT_E);
	grid_port_receive_task(&GRID_PORT_S);
	grid_port_receive_task(&GRID_PORT_W);	
		

}

static void ui_task_inner(){
	

	// every other entry of the superloop
	if (loopcount%4==0){


		grid_port_ping_try_everywhere();

		// IF LOCAL MESSAGE IS AVAILABLE
		if (grid_ui_event_count_istriggered_local(&grid_ui_state)){

			CRITICAL_SECTION_ENTER()
			grid_port_process_ui_local_UNSAFE(&grid_ui_state); // COOLDOWN DELAY IMPLEMENTED INSIDE
			CRITICAL_SECTION_LEAVE()
		
		}


		// Bandwidth Limiter for Broadcast messages

		
		
		if (grid_ui_state.port->cooldown > 0){
			grid_ui_state.port->cooldown--;
		}
		
		
		if (grid_ui_state.port->cooldown > 5){
			printf("SKIP\r\n");

		}
		else{

			if (grid_ui_event_count_istriggered(&grid_ui_state)){

				grid_ui_state.port->cooldown += 3;	

				CRITICAL_SECTION_ENTER()
				grid_port_process_ui_UNSAFE(&grid_ui_state); 
				CRITICAL_SECTION_LEAVE()
			}
		}


	}

}


static void inbound_task_inner(){
		
	/* ========================= GRID INBOUND TASK ============================= */						

	
	// Copy data from UI_RX to HOST_TX & north TX AND STUFF
	grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback
	
	grid_port_process_inbound(&GRID_PORT_N, 0);		
	grid_port_process_inbound(&GRID_PORT_E, 0);		
	grid_port_process_inbound(&GRID_PORT_S, 0);
	grid_port_process_inbound(&GRID_PORT_W, 0);
	
	grid_port_process_inbound(&GRID_PORT_H, 0);	// USB	



}

static void outbound_task_inner(){
	
		
	/* ========================= GRID OUTBOUND TASK ============================= */	
	
	// If previous xfer is completed and new data is available then move data from txbuffer to txdoublebuffer and start new xfer.
	grid_port_process_outbound_usart(&GRID_PORT_N);
	grid_port_process_outbound_usart(&GRID_PORT_E);
	grid_port_process_outbound_usart(&GRID_PORT_S);
	grid_port_process_outbound_usart(&GRID_PORT_W);
	
	// Translate grid messages to usb messages and xfer them to the host
	grid_port_process_outbound_usb(&GRID_PORT_H);
	
	// Translate grid messages to ui commands (LED)
	grid_port_process_outbound_ui(&GRID_PORT_U);

}


static void led_task_inner(){


	if (RTC1MS*10 < grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_led_get_tick_lastrealtime(&grid_led_state))){
	
		grid_led_set_tick_lastrealtime(&grid_led_state, grid_sys_rtc_get_time(&grid_sys_state));

		grid_led_tick(&grid_led_state);
		
		grid_led_render_framebuffer(&grid_led_state);	

		grid_d51_led_generate_frame(&grid_d51_led_state, &grid_led_state);

		grid_d51_led_start_transfer(&grid_d51_led_state);

	}
	

}

volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;
volatile uint8_t reportflag = 0;
volatile uint8_t heartbeatflag = 0;



static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_realtime_ms;
static struct timer_task RTC_Scheduler_heartbeat;
static struct timer_task RTC_Scheduler_report;


void RTC_Scheduler_ping_cb(const struct timer_task *const timer_task)
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

void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{

	grid_sys_rtc_tick_time(&grid_sys_state);	

}

void RTC_Scheduler_realtime_millisecond_cb(const struct timer_task *const timer_task)
{

	grid_ui_rtc_ms_tick_time(&grid_ui_state);	
	grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, !gpio_get_pin_level(MAP_MODE));	

}



void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{

	heartbeatflag = 1;

}

void RTC_Scheduler_report_cb(const struct timer_task *const timer_task)
{
	reportflag = 1;
}


void init_timer(void)
{
	
	RTC_Scheduler_ping.interval = RTC1MS*GRID_PARAMETER_PING_interval;
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_heartbeat.interval = RTC1MS*GRID_PARAMETER_HEARTBEAT_interval;
	RTC_Scheduler_heartbeat.cb       = RTC_Scheduler_heartbeat_cb;
	RTC_Scheduler_heartbeat.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_realtime.interval = 1;
	RTC_Scheduler_realtime.cb       = RTC_Scheduler_realtime_cb;
	RTC_Scheduler_realtime.mode     = TIMER_TASK_REPEAT;

	RTC_Scheduler_realtime_ms.interval = RTC1MS*1;
	RTC_Scheduler_realtime_ms.cb       = RTC_Scheduler_realtime_millisecond_cb;
	RTC_Scheduler_realtime_ms.mode     = TIMER_TASK_REPEAT;

	RTC_Scheduler_report.interval = RTC1MS*1000;
	RTC_Scheduler_report.cb       = RTC_Scheduler_report_cb;
	RTC_Scheduler_report.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_heartbeat);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime_ms);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);
	
	timer_start(&RTC_Scheduler);
	
}

//====================== USB TEST =====================//


enum SYS_I2C_STATUS{
	SYS_I2C_STATUS_BUSY,
	SYS_I2C_STATUS_INIT,
	SYS_I2C_STATUS_TXC,
	SYS_I2C_STATUS_RXC,
	SYS_I2C_STATUS_ERR,
	SYS_I2C_STATUS_TRAP
};

static uint8_t SYS_I2C_example_str[12] = "Hello World!";
struct io_descriptor *SYS_I2C_io;
volatile uint8_t sys_i2c_done_flag = SYS_I2C_STATUS_INIT;
volatile uint8_t sys_i2c_enabled = 0;


void SYS_I2C_tx_complete_callback(struct i2c_m_async_desc *const i2c)
{

	printf("$");
	sys_i2c_done_flag = SYS_I2C_STATUS_TXC;

	i2c_m_async_send_stop(i2c);

}

void SYS_I2C_rx_complete_callback(struct i2c_m_async_desc *const i2c)
{
	printf("#");

	sys_i2c_done_flag = SYS_I2C_STATUS_RXC;

}

void SYS_I2C_error_callback(struct i2c_m_async_desc *const i2c, int32_t error)
{
	printf("@");

	i2c_m_async_send_stop(i2c);
	sys_i2c_done_flag = SYS_I2C_STATUS_ERR;


}

uint32_t SYS_I2C_start(void)
{

	i2c_m_async_get_io_descriptor(&SYS_I2C, &SYS_I2C_io);

	uint32_t ret = i2c_m_async_enable(&SYS_I2C);


	i2c_m_async_register_callback(&SYS_I2C, I2C_M_ASYNC_TX_COMPLETE, (FUNC_PTR)SYS_I2C_tx_complete_callback);
	i2c_m_async_register_callback(&SYS_I2C, I2C_M_ASYNC_RX_COMPLETE, (FUNC_PTR)SYS_I2C_rx_complete_callback);
	i2c_m_async_register_callback(&SYS_I2C, I2C_M_ASYNC_ERROR, (FUNC_PTR)SYS_I2C_error_callback);

	return ret;


}


// QSPI
static uint8_t buf[16] = {0x0};

static void qspi_xfer_complete_cb(struct _dma_resource *resource)
{
	/* Transfer completed */
	printf("QSPI XFER DONE! ");

	for (uint8_t i=0; i<16; i++){

		printf("0x%02x ", buf[i]);
	}
	printf("\r\n");
}

/**
 * Example of using QSPI_INSTANCE to get N25Q256A status value,
 * and check bit 0 which indicate embedded operation is busy or not.
 */
void qspi_test(void)
{
	struct _qspi_command cmd = {
	    .inst_frame.bits.inst_en      = 1,
	    .inst_frame.bits.data_en      = 1,
	    .inst_frame.bits.addr_en      = 1,
	    .inst_frame.bits.dummy_cycles = 8,
	    .inst_frame.bits.tfr_type     = QSPI_READMEM_ACCESS,
	    .instruction                  = 0x0B,
	    .address                      = 0,
	    .buf_len                      = 14,
	    .rx_buf                       = buf,
	};

	qspi_dma_register_callback(&QSPI_INSTANCE, QSPI_DMA_CB_XFER_DONE, qspi_xfer_complete_cb);
	qspi_dma_enable(&QSPI_INSTANCE);
	qspi_dma_serial_run_command(&QSPI_INSTANCE, &cmd);
	
}




int main(void)
{

	// boundary scan here
	uint32_t boundary_result[4] = {0};
	grid_d51_boundary_scan(boundary_result); // must run before atmel_start_init sets up gpio

	atmel_start_init();	// this sets up gpio and printf
	
   	grid_platform_printf("Start Initialized %d %s\r\n", 123, "Cool!");

	grid_d51_init(); // Check User Row

	grid_sys_init(&grid_sys_state);
	grid_msg_init(&grid_msg_state);

	grid_d51_boundary_scan_report(boundary_result);

            
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ){

		if (SYS_I2C_start() == ERR_NONE){
			sys_i2c_enabled = 1;
			printf("I2C init OK!\r\n");
		}
		else{
			printf("I2C init FAILED!\r\n");
		}

	}
	else{

		printf("I2C UNSUPPORTED!\r\n");
	}

	printf("QSPI\r\n");
	qspi_test();

	// grid_d51_nvm_erase_all(&grid_d51_nvm_state);

	

		
	printf("Hardware test complete");


	grid_lua_init(&grid_lua_state);
	grid_lua_start_vm(&grid_lua_state);


	audiodf_midi_init();

	composite_device_start();



	grid_d51_usb_init();

	grid_usb_midi_buffer_init();

	grid_usb_keyboard_buffer_init(&grid_keyboard_state);
		

	//  x/512xb 0x80000
	grid_module_common_init();

	grid_d51_led_init(&grid_d51_led_state, &grid_led_state);

	if (sys_i2c_enabled){
		uint8_t id = grid_fusb302_read_id(SYS_I2C_io);
	}

	printf("Start TOC init\r\n");
	grid_d51_nvm_toc_init(&grid_d51_nvm_state);
	//grid_d51_nvm_toc_debug(&grid_d51_nvm_state);
	printf("Done TOC init\r\n");
	grid_ui_page_load(&grid_ui_state, 0); //load page 0

	while (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state))
	{
		grid_ui_bulk_pageread_next(&grid_ui_state);
	}
	
	//grid_d51_nvm_toc_debug(&grid_d51_nvm_state);
	
	init_timer();

	uint32_t loopstart = 0;




	#ifdef GRID_BUILD_UNKNOWN
		printf("\r\n##Build: Unknown##\r\n\r\n");
	#endif
	#ifdef GRID_BUILD_NIGHTLY
		printf("\r\n##Build: Nightly##\r\n\r\n");
	#endif
	#ifdef GRID_BUILD_DEBUG
		printf("\r\n##Build: Debug##\r\n\r\n");
	#endif
	#ifdef GRID_BUILD_RELEASE
		printf("\r\n##Build: Release##\r\n\r\n");
	#endif



	while (1) {


		// struct _qspi_command cmd = {
		// 	.inst_frame.bits.inst_en      = 1,
		// 	.inst_frame.bits.data_en      = 1,
		// 	.inst_frame.bits.addr_en      = 1,
		// 	.inst_frame.bits.dummy_cycles = 8,
		// 	.inst_frame.bits.tfr_type     = QSPI_READMEM_ACCESS,
		// 	.instruction                  = 0x0B,
		// 	.address                      = 0,
		// 	.buf_len                      = 14,
		// 	.rx_buf                       = buf,
		// };

		// qspi_dma_serial_run_command(&QSPI_INSTANCE, &cmd);


		
		if (usb_d_get_frame_num() != 0){
			
			if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1){
			
				printf("USB CONNECTED\r\n\r\n");
				printf("HWCFG %d\r\n", grid_sys_get_hwcfg(&grid_sys_state));

				grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 100);	
				grid_led_set_alert_frequency(&grid_led_state, -2);	
				grid_led_set_alert_phase(&grid_led_state, 200);	
				
				grid_msg_set_heartbeat_type(&grid_msg_state, 1);

		
			}

		}
			
		//printf("WTF\r\n\r\n");
		
		loopcounter++;
		loopcount++;

		if (reportflag){

				reportflag = 0;
				loopcount = 0;
		}

		if (loopcounter == 1000){

			grid_ui_state.ui_interaction_enabled = 1;

			grid_d51_nvic_debug_priorities();
		}

		//Touch Chip
		if (sys_i2c_enabled && loopcounter%100 == 0){
			grid_mxt144u_read_id(SYS_I2C_io);
			//grid_fusb302_read_id(SYS_I2C_io);
		}
		

		usb_task_inner();
	
	
		nvm_task_inner();
		
		receive_task_inner();

		//lua_gc(grid_lua_state.L, LUA_GCSTOP);



		ui_task_inner();

	
		outbound_task_inner();

		inbound_task_inner();


		led_task_inner();



		if (heartbeatflag){

			heartbeatflag = 0;

			grid_protocol_send_heartbeat();


		}

		

		if (grid_sys_get_editor_connected_state(&grid_sys_state) == 1){

			

			if (grid_sys_rtc_get_elapsed_time(&grid_sys_state, grid_msg_get_editor_heartbeat_lastrealtime(&grid_msg_state))>2000*RTC1MS){

				printf("EDITOR timeout\r\n");

				grid_sys_set_editor_connected_state(&grid_sys_state, 0);

				grid_ui_state.page_change_enabled = 1;

			}

		}


	}//WHILE



}//MAIN

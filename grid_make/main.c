

#include "grid/grid_module.h"

#include "grid/grid_ui.h"

#include <atmel_start.h>
#include "atmel_start_pins.h"

#include <hal_qspi_dma.h>


#include <stdio.h>

#include <string.h>
#include <hpl_reset.h>


#include "hal_rtos.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#include "usb/class/midi/device/audiodf_midi.h"
/* GetIdleTaskMemory prototype (linked to static allocation support) */

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configMINIMAL_STACK_SIZE];


void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
                                          uint32_t *pulTimerTaskStackSize){
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                                          uint32_t *pulIdleTaskStackSize){
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName ){

	CRITICAL_SECTION_ENTER()

	while(1){
		printf("Stack Overflow %s\r\n", pcTaskGetName(xTask));
		delay_ms(1000);
	}

	CRITICAL_SECTION_LEAVE()


}

static TaskHandle_t      		xCreatedUiTask;
#define TASK_UI_STACK_SIZE 		(512 / sizeof(portSTACK_TYPE))
#define TASK_UI_PRIORITY 		(1)

static TaskHandle_t      		xCreatedUsbTask;
#define TASK_USB_STACK_SIZE 	(512 / sizeof(portSTACK_TYPE))
#define TASK_USB_PRIORITY 		(2)

static TaskHandle_t      		xCreatedNvmTask;
#define TASK_NVM_STACK_SIZE 	(512 / sizeof(portSTACK_TYPE))
#define TASK_NVM_PRIORITY 		(2)

static TaskHandle_t      			xCreatedReceiveTask;
#define TASK_RECEIVE_STACK_SIZE 	(512 / sizeof(portSTACK_TYPE))
#define TASK_RECEIVE_PRIORITY 		(2)

static TaskHandle_t      			xCreatedInboundTask;
#define TASK_INBOUND_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_INBOUND_PRIORITY 		(2)

static TaskHandle_t      			xCreatedOutboundTask;
#define TASK_OUTBOUND_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_OUTBOUND_PRIORITY 		(2)

static TaskHandle_t      		xCreatedLedTask;
#define TASK_LED_STACK_SIZE 	(4*1024 / sizeof(portSTACK_TYPE))
#define TASK_LED_PRIORITY 		(4)


#define TASK_EXAMPLE_STACK_SIZE (512 / sizeof(portSTACK_TYPE))
#define TASK_EXAMPLE_STACK_PRIORITY (tskIDLE_PRIORITY + 2)
#define TASK_EXAMPLE2_STACK_PRIORITY (tskIDLE_PRIORITY + 3)

static TaskHandle_t      xCreatedExampleTask;
static TaskHandle_t      xCreatedExample2Task;

static SemaphoreHandle_t disp_mutex;

// StaticTask_t xTaskBufferUi;
// StackType_t xStackUi[TASK_UI_STACK_SIZE];

// StaticTask_t xTaskBufferUsb;
// StackType_t xStackUsb[TASK_USB_STACK_SIZE];

// StaticTask_t xTaskBufferNvm;
// StackType_t xStackNvm[TASK_NVM_STACK_SIZE];

// StaticTask_t xTaskBufferReceive;
// StackType_t xStackReceive[TASK_RECEIVE_STACK_SIZE];

// StaticTask_t xTaskBufferInbound;
// StackType_t xStackInbound[TASK_INBOUND_STACK_SIZE];

// StaticTask_t xTaskBufferOutbound;
// StackType_t xStackOutbound[TASK_OUTBOUND_STACK_SIZE];

// StaticTask_t xTaskBufferLed;
// StackType_t xStackLed[TASK_LED_STACK_SIZE];


/**
 * OS example task
 *
 * \param[in] p The void pointer for OS task Standard model.
 *
 */

volatile uint32_t globaltest = 0;



volatile uint8_t midi_rx_buffer[16] = {0};

static void usb_task_inner(struct grid_d51_task* task){

	grid_d51_task_start(task);

	grid_keyboard_tx_pop();
	
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

		printf("MIDI: %02x %02x %02x %02x\n", midi_rx_buffer[0],midi_rx_buffer[1],midi_rx_buffer[2],midi_rx_buffer[3]);
		
		uint8_t message[30] = {0};
			
		sprintf(message, "MIDI: %02x %02x %02x %02x\n", midi_rx_buffer[0],midi_rx_buffer[1],midi_rx_buffer[2],midi_rx_buffer[3]);
		
		grid_debug_print_text(message);

		for (uint8_t i=0; i<16; i++){

			midi_rx_buffer[i] = 0;

		}


	}	
	
	// SERIAL READ 

	cdcdf_acm_read(GRID_PORT_H.rx_double_buffer, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);			
	
	// itt lesz a baj: circ buffer k�ne
	uint16_t usblength = strlen(GRID_PORT_H.rx_double_buffer);
	
	if (usblength){	

		GRID_PORT_H.rx_double_buffer_status = 1;			
		GRID_PORT_H.rx_double_buffer_read_start_index = 0;
		GRID_PORT_H.rx_double_buffer_seek_start_index = usblength-3; //-3

		//grid_port_receive_decode(&GRID_PORT_H, 0, usblength-2);
		grid_port_receive_task(&GRID_PORT_H);
		
		//clear buffer otherwise strlen might fail
		for(uint32_t i=0; i<usblength; i++){
			
			GRID_PORT_H.rx_double_buffer[i] = 0;
		}
			
	}


	grid_d51_task_stop(task);
}

static void nvm_task_inner(struct grid_d51_task* task){

	grid_d51_task_start(task);

	// NVM BULK READ
	
	if (GRID_PORT_U.rx_double_buffer_status == 0){
		
		if (grid_nvm_ui_bulk_read_is_in_progress(&grid_nvm_state, &grid_ui_state)){
			
			grid_nvm_ui_bulk_read_next(&grid_nvm_state, &grid_ui_state);
			
			
		}	
		
	}
	
	// NVM BULK CLEAR
	
	if (grid_nvm_ui_bulk_clear_is_in_progress(&grid_nvm_state, &grid_ui_state)){
		
		grid_nvm_ui_bulk_clear_next(&grid_nvm_state, &grid_ui_state);
		
		
	}
	
	// NVM BULK STORE
	
	if (grid_nvm_ui_bulk_store_is_in_progress(&grid_nvm_state, &grid_ui_state)){
		
		printf("HI\r\n");
		grid_nvm_ui_bulk_store_next(&grid_nvm_state, &grid_ui_state);
			
		
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
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++)
	{
		GRID_PORT_U.rx_double_buffer[i] = 0;
	}

	grid_d51_task_stop(task);
}

static void receive_task_inner(struct grid_d51_task* task){

	grid_d51_task_start(task);	

	grid_port_receive_task(&GRID_PORT_N);
	grid_port_receive_task(&GRID_PORT_E);
	grid_port_receive_task(&GRID_PORT_S);
	grid_port_receive_task(&GRID_PORT_W);	

	
	grid_d51_task_stop(task);						

}

static void ui_task_inner(struct grid_d51_task* task){
	

	grid_d51_task_start(task);

	grid_port_process_ui(&grid_ui_state, &GRID_PORT_U); // COOLDOWN DELAY IMPLEMENTED INSIDE

	grid_d51_task_stop(task);
}


static void inbound_task_inner(struct grid_d51_task* task){
		
	/* ========================= GRID INBOUND TASK ============================= */						

	grid_d51_task_start(task);
	
	// Copy data from UI_RX to HOST_TX & north TX AND STUFF
	grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback
	
	grid_port_process_inbound(&GRID_PORT_N, 0);		
	grid_port_process_inbound(&GRID_PORT_E, 0);		
	grid_port_process_inbound(&GRID_PORT_S, 0);
	grid_port_process_inbound(&GRID_PORT_W, 0);
	
	grid_port_process_inbound(&GRID_PORT_H, 0);	// USB	


	grid_d51_task_stop(task);

}

static void outbound_task_inner(struct grid_d51_task* task){
	
		
	grid_d51_task_start(task);
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

	grid_d51_task_stop(task);
}


static void led_task_inner(struct grid_d51_task* task){

	grid_d51_task_start(task);

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
	

	grid_led_tick(&grid_led_state);


	
	grid_led_lowlevel_render_all(&grid_led_state);	
				
// 	 		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
// 	
// 	 		}
	grid_led_lowlevel_hardware_start_transfer(&grid_led_state);

	
	grid_d51_task_stop(task);	

}

static void usb_task(void *p)
{
	(void)p;

	while (1) {

		usb_task_inner(NULL);
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}

}

static void nvm_task(void *p){

	(void)p;

	while (1) {

		nvm_task_inner(NULL);
		vTaskDelay(1*configTICK_RATE_HZ/1000);
	}

}


static void ui_task(void *p){

	(void)p;

	while (1) {

		ui_task_inner(NULL);
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}
}

static void receive_task(void *p){

	(void)p;

	while (1) {
			
		receive_task_inner(NULL);
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}
}

static void inbound_task(void *p){

	(void)p;

	while (1) {	
		vTaskDelay(1*configTICK_RATE_HZ/1000);
	}
}


static void outbound_task(void *p){

	(void)p;
	while (1) {	
		vTaskDelay(1*configTICK_RATE_HZ/1000);
	}
}


static void led_task(void *p)
{
	(void)p;
	while (1) {

		globaltest++;
		inbound_task_inner(NULL);
		outbound_task_inner(NULL);

		led_task_inner(NULL);


		vTaskDelay(1*configTICK_RATE_HZ/1000);

		//os_sleep(400);

	}
}


volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;
volatile uint8_t reportflag = 0;

static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
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
	grid_task_timer_tick(&grid_task_state);
			
	uint8_t mapmode_value = !gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != grid_sys_state.mapmodestate){
		
		grid_sys_state.mapmodestate = mapmode_value;
			
		if (grid_sys_state.mapmodestate == 0){ // RELEASE
			
				
		}
		else{ // PRESS

			struct grid_ui_element* sys_ele = &grid_ui_state.element_list[grid_ui_state.element_list_length-1]; 

			struct grid_ui_event* eve = grid_ui_event_find(sys_ele, GRID_UI_EVENT_MAPMODE_CHANGE);
			
			if (eve == NULL){
				printf("NOT FOUND!\r\n");
			}
			else{

				printf("FOUND!\r\n");
			}
			
			
			grid_ui_event_trigger(eve);		


		}

	}

}


void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{

	struct grid_msg response;

	grid_msg_init(&response);
	grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

	uint8_t temp[30] = {0};
	sprintf(temp, GRID_CLASS_HEARTBEAT_frame);
	grid_msg_body_append_text(&response, temp);

	grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

	grid_msg_text_set_parameter(&response, 0, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, grid_sys_state.heartbeat_type);
	grid_msg_text_set_parameter(&response, 0, GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, grid_sys_get_hwcfg(&grid_sys_state));
	grid_msg_text_set_parameter(&response, 0, GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
	grid_msg_text_set_parameter(&response, 0, GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
	grid_msg_text_set_parameter(&response, 0, GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

	grid_msg_packet_close(&response);
	grid_msg_packet_send_everywhere(&response);

}

void RTC_Scheduler_report_cb(const struct timer_task *const timer_task)
{
	reportflag = 1;
}


void init_timer(void)
{
	
		
	//RTC_Scheduler_ping.interval = RTC1SEC/20; //50ms
	RTC_Scheduler_ping.interval = RTC1MS*GRID_PARAMETER_PING_interval;
	RTC_Scheduler_ping.cb       = RTC_Scheduler_ping_cb;
	RTC_Scheduler_ping.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_heartbeat.interval = RTC1MS*GRID_PARAMETER_HEARTBEAT_interval;
	RTC_Scheduler_heartbeat.cb       = RTC_Scheduler_heartbeat_cb;
	RTC_Scheduler_heartbeat.mode     = TIMER_TASK_REPEAT;
	
	
	RTC_Scheduler_realtime.interval = 1;
	RTC_Scheduler_realtime.cb       = RTC_Scheduler_realtime_cb;
	RTC_Scheduler_realtime.mode     = TIMER_TASK_REPEAT;

	RTC_Scheduler_report.interval = RTC1MS*100;
	RTC_Scheduler_report.cb       = RTC_Scheduler_report_cb;
	RTC_Scheduler_report.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_heartbeat);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
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
	
    GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "Start Initialized");

	grid_d51_init(); // Check User Row

	grid_sys_init(&grid_sys_state);

	grid_d51_boundary_scan_report(boundary_result);

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "D51 Init");
            
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

	// grid_nvm_erase_all(&grid_nvm_state);


	if (sys_i2c_enabled){
		uint8_t id = grid_fusb302_read_id(SYS_I2C_io);
	}
		
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "Hardware test complete");

	grid_expr_init(&grid_expr_state);

	grid_lua_init(&grid_lua_state);
	grid_lua_start_vm(&grid_lua_state);

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "LUA init complete");

	audiodf_midi_init();

	composite_device_start();

	grid_usb_serial_init();
	grid_usb_midi_init();

	grid_keyboard_init(&grid_keyboard_state);
		
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Initialized");
		
		

	


	// Init Bank Color Bug when config was previously saved

	// xCreatedUsbTask = xTaskCreateStatic(usb_task, "Usb Task", TASK_USB_STACK_SIZE, ( void * ) 1, TASK_USB_PRIORITY, xStackUsb, &xTaskBufferUsb);
	// xCreatedNvmTask = xTaskCreateStatic(nvm_task, "Nvm Task", TASK_NVM_STACK_SIZE, ( void * ) 1, TASK_NVM_PRIORITY, xStackNvm, &xTaskBufferNvm);
	// xCreatedUiTask = xTaskCreateStatic(ui_task, "Ui Task",  TASK_UI_STACK_SIZE, ( void * ) 1, TASK_UI_PRIORITY, xStackUi, &xTaskBufferUi);
	// xCreatedReceiveTask = xTaskCreateStatic(receive_task, "Rec Task", TASK_RECEIVE_STACK_SIZE, ( void * ) 1, TASK_RECEIVE_PRIORITY, xStackReceive, &xTaskBufferReceive);
	// xCreatedInboundTask = xTaskCreateStatic(inbound_task, "Inb Task", TASK_INBOUND_STACK_SIZE, ( void * ) 1, TASK_INBOUND_PRIORITY, xStackInbound, &xTaskBufferInbound);
	// xCreatedOutboundTask = xTaskCreateStatic(outbound_task, "Outb Task", TASK_OUTBOUND_STACK_SIZE, ( void * ) 1, TASK_OUTBOUND_PRIORITY, xStackOutbound, &xTaskBufferOutbound);
	// xCreatedLedTask = xTaskCreateStatic(led_task, "Led Task", TASK_LED_STACK_SIZE, ( void * ) 1, TASK_LED_PRIORITY, xStackLed, &xTaskBufferLed);


	//  x/512xb 0x80000
	grid_module_common_init();
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Grid Module Initialized");
	grid_nvm_toc_init(&grid_nvm_state);
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "TOC Initialized");
	grid_ui_page_load(&grid_ui_state, &grid_nvm_state, 0); //load page 0;

	while (grid_nvm_ui_bulk_read_is_in_progress(&grid_nvm_state, &grid_ui_state))
	{
		grid_nvm_ui_bulk_read_next(&grid_nvm_state, &grid_ui_state);
	}
	

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "UI Page0 loaded");

	// grid_sys_nvm_load_configuration(&grid_sys_state, &grid_nvm_state);
	// grid_ui_nvm_load_all_configuration(&grid_ui_state, &grid_nvm_state);	
	
//	grid_nvm_config_mock(&grid_nvm_state);
//	grid_nvm_config_mock(&grid_nvm_state);
//	grid_nvm_config_mock(&grid_nvm_state);

	grid_nvm_toc_debug(&grid_nvm_state);
	
//	grid_nvm_toc_defragmant(&grid_nvm_state);


	// init_timer is last before loop because it creates interrupts
	init_timer();

	uint32_t loopcounter = 1;
	uint32_t loopstart = 0;
	uint8_t usb_init_flag = 0;	

	uint8_t task_list_length = 6;
	struct grid_d51_task task_list[task_list_length];

	struct grid_d51_task* grid_usb_task = &task_list[0];
	struct grid_d51_task* grid_nvm_task = &task_list[1];
	struct grid_d51_task* grid_receive_task = &task_list[2];
	struct grid_d51_task* grid_ui_task = &task_list[3];
	struct grid_d51_task* grid_inbound_task = &task_list[4];
	struct grid_d51_task* grid_outbound_task = &task_list[5];
	struct grid_d51_task* grid_led_task = &task_list[6];

	grid_d51_task_init(grid_usb_task, 		"usb");
	grid_d51_task_init(grid_nvm_task, 		"nvm");
	grid_d51_task_init(grid_receive_task, 	"rec");
	grid_d51_task_init(grid_ui_task, 		"ui ");
	grid_d51_task_init(grid_inbound_task, 	"in ");
	grid_d51_task_init(grid_outbound_task, 	"out");
	grid_d51_task_init(grid_led_task, 		"led");


	grid_ui_state.task = grid_ui_task;


	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Entering Main Loop");

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


		
		if (usb_init_flag == 0){
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{			
			
				grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN	
				
				GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Connected");
				
				grid_sys_state.heartbeat_type = 1;

				usb_init_flag = 1;
		
			}
			
		}

		
		loopcounter++;

		if (loopcounter == 1000){


			// printf("vTaskStartScheduler! \r\n");
			// delay_ms(2);
			// vTaskStartScheduler();

		}


		if (sys_i2c_enabled){
			grid_mxt144u_read_id(SYS_I2C_io);
		}
		

		

		usb_task_inner(grid_usb_task);
	
		nvm_task_inner(grid_nvm_task);
		
		receive_task_inner(grid_receive_task);

		//lua_gc(grid_lua_state.L, LUA_GCSTOP);



		ui_task_inner(grid_ui_task);

		inbound_task_inner(grid_inbound_task);

		outbound_task_inner(grid_outbound_task);

		led_task_inner(grid_led_task);


		//grid_lua_debug_memory_stats(&grid_lua_state, "Ui");
		lua_gc(grid_lua_state.L, LUA_GCCOLLECT);


		


		if (reportflag){


			reportflag = 0;


			uint8_t reportbuffer[300] = {0};
			uint16_t length = 0;



			sprintf(&reportbuffer[length], GRID_CLASS_DEBUGTASK_frame_start);
			length += strlen(&reportbuffer[length]);

			for (uint8_t i=0; i<task_list_length; i++){

				struct grid_d51_task* task = &task_list[i];

				//printf("%s %d ", task->taskname, task->subtaskcount);
				for (uint8_t j=0; j<task->subtaskcount; j++){

					sprintf(&reportbuffer[length], "!%s,%d,%d,%d", task->taskname, task->min[j]/120, task->sum[j]/120/task->startcount, task->max[j]/120);
					length += strlen(&reportbuffer[length]);

				}

				grid_d51_task_clear(task);
			}

			//printf("\r\n");


			sprintf(&reportbuffer[length], GRID_CLASS_EVENTPREVIEW_frame_end);
			length += strlen(&reportbuffer[length]);



			struct grid_msg response;
									
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			grid_msg_body_append_text(&response, reportbuffer);
				

			grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
			grid_msg_text_set_parameter(&response, 0, GRID_CLASS_EVENTPREVIEW_LENGTH_offset, GRID_CLASS_EVENTPREVIEW_LENGTH_length, length-GRID_CLASS_DEBUGTASK_OUTPUT_offset);
			


			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);			

		
			receive_task_inner(NULL);
			//ui_task_inner(grid_ui_task);
			inbound_task_inner(NULL);
			outbound_task_inner(NULL);

			//lua_getglobal(grid_lua_state.L, "print");


		    //grid_lua_dostring(&grid_lua_state, "ele[0].ec()");

		    //grid_lua_dostring(&grid_lua_state, "print(glr(), glg(), glb())");

		
			//grid_lua_dostring(&grid_lua_state, grid_ui_state.element_list[0].event_list[2].action_call);

		}

	}//WHILE



}//MAIN

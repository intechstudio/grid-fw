

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

static void usb_task_inner(){


	
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


}



static void nvm_task_inner(){

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

}

static void receive_task_inner(){
			
	grid_port_receive_task(&GRID_PORT_N);
	grid_port_receive_task(&GRID_PORT_E);
	grid_port_receive_task(&GRID_PORT_S);
	grid_port_receive_task(&GRID_PORT_W);							

}

static void ui_task_inner(){
	
	grid_port_process_ui(&GRID_PORT_U); // COOLDOWN DELAY IMPLEMENTED INSIDE

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


	
	grid_led_lowlevel_render_all(&grid_led_state);	
				
// 	 		while(grid_led_hardware_is_transfer_completed(&grid_led_state) != 1){
// 	
// 	 		}
	grid_led_lowlevel_hardware_start_transfer(&grid_led_state);

	
		

}

static void usb_task(void *p)
{
	(void)p;

	while (1) {

		usb_task_inner();
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}

}

static void nvm_task(void *p){


	(void)p;

	while (1) {

		nvm_task_inner();
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}

}


static void ui_task(void *p){

	(void)p;

	while (1) {




		ui_task_inner();
			
		vTaskDelay(1*configTICK_RATE_HZ/1000);

	}
}

static void receive_task(void *p){

	(void)p;

	while (1) {
			
		
		receive_task_inner();
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
		

		

		inbound_task_inner();
		outbound_task_inner();





		led_task_inner();


		vTaskDelay(1*configTICK_RATE_HZ/1000);

		//os_sleep(400);

	}
}


volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;


static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_heartbeat;

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

volatile uint8_t heartbeat_enable = 1;

void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{

	grid_sys_rtc_tick_time(&grid_sys_state);	
	grid_task_timer_tick(&grid_task_state);
			
	uint8_t mapmode_value = !gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != grid_sys_state.mapmodestate){
		
		grid_sys_state.mapmodestate = mapmode_value;
			
		if (grid_sys_state.mapmodestate == 0){ // RELEASE
			
			grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_MAPMODE_RELEASE);
			heartbeat_enable = !heartbeat_enable;

				
		}
		else{ // PRESS
			
			grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_MAPMODE_PRESS);		
												 
		}

	}

}


void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{
	if (heartbeat_enable){

		grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_HEARTBEAT);

	}
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

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_heartbeat);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
	
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


	grid_d51_boundary_scan(boundary_result);



	atmel_start_init();	
    
	grid_d51_dwt_enable();
            
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "Start Initialized");

	grid_lua_init(&grid_lua_state);
	grid_lua_start_vm(&grid_lua_state);

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "LUA init complete");

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

	printf("test.mcu.ATSAMD51N20A\r\n");
	printf("test.hwcfg.%d\r\n", grid_sys_get_hwcfg(&grid_sys_state));

	uint32_t uniqueid[4] = {0};
	grid_sys_get_id(uniqueid);	

	printf("test.serialno.%08x %08x %08x %08x\r\n", uniqueid[0], uniqueid[1], uniqueid[2], uniqueid[3]);

	for (uint8_t i=0; i<4; i++){

		delay_ms(10);
		printf("test.boundary.%d.", i);

		for (uint8_t j=0; j<32; j++){

			if (boundary_result[i]&(1<<j)){
				printf("1");
			}
			else{
				printf("0");
			}
		}


		printf("\r\n");

	}



	printf("Hello %d %d %d %d", boundary_result[0], boundary_result[1], boundary_result[2], boundary_result[3]);

	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "D51 Init");
	grid_d51_init(); // Check User Row



	audiodf_midi_init();

	composite_device_start();


	grid_usb_serial_init();
	//grid_usb_midi_init();
	grid_usb_midi_init();

	grid_keyboard_init(&grid_keyboard_state);
		
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Initialized");
		
		
	rand_sync_enable(&RAND_0);	
		
	grid_expr_init(&grid_expr_state);

	//NVIC_SystemReset();


	grid_module_common_init();
    grid_ui_reinit(&grid_ui_state);
 				
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Grid Module Initialized");

	init_timer();

	
	
	uint32_t loopcounter = 0;
	
	uint32_t loopstart = 0;

	uint8_t usb_init_flag = 0;	



	// Init Bank Color Bug when config was previously saved
	
	grid_sys_nvm_load_configuration(&grid_sys_state, &grid_nvm_state);
	grid_ui_nvm_load_all_configuration(&grid_ui_state, &grid_nvm_state);	
	

	// xCreatedUsbTask = xTaskCreateStatic(usb_task, "Usb Task", TASK_USB_STACK_SIZE, ( void * ) 1, TASK_USB_PRIORITY, xStackUsb, &xTaskBufferUsb);
	// xCreatedNvmTask = xTaskCreateStatic(nvm_task, "Nvm Task", TASK_NVM_STACK_SIZE, ( void * ) 1, TASK_NVM_PRIORITY, xStackNvm, &xTaskBufferNvm);
	// xCreatedUiTask = xTaskCreateStatic(ui_task, "Ui Task",  TASK_UI_STACK_SIZE, ( void * ) 1, TASK_UI_PRIORITY, xStackUi, &xTaskBufferUi);
	// xCreatedReceiveTask = xTaskCreateStatic(receive_task, "Rec Task", TASK_RECEIVE_STACK_SIZE, ( void * ) 1, TASK_RECEIVE_PRIORITY, xStackReceive, &xTaskBufferReceive);
	// xCreatedInboundTask = xTaskCreateStatic(inbound_task, "Inb Task", TASK_INBOUND_STACK_SIZE, ( void * ) 1, TASK_INBOUND_PRIORITY, xStackInbound, &xTaskBufferInbound);
	// xCreatedOutboundTask = xTaskCreateStatic(outbound_task, "Outb Task", TASK_OUTBOUND_STACK_SIZE, ( void * ) 1, TASK_OUTBOUND_PRIORITY, xStackOutbound, &xTaskBufferOutbound);
	// xCreatedLedTask = xTaskCreateStatic(led_task, "Led Task", TASK_LED_STACK_SIZE, ( void * ) 1, TASK_LED_PRIORITY, xStackLed, &xTaskBufferLed);


	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Entering Main Loop");

	//  x/512xb 0x80000

	grid_nvm_toc_init(&grid_nvm_state);

	grid_nvm_config_mock(&grid_nvm_state);
	grid_nvm_config_mock(&grid_nvm_state);
	grid_nvm_config_mock(&grid_nvm_state);

	grid_nvm_toc_debug(&grid_nvm_state);

	// grid_nvm_toc_defragmant(&grid_nvm_state);

	printf("QSPI\r\n");
	qspi_test();

//	grid_nvm_erase_all(&grid_nvm_state);


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
				
				grid_sys_set_bank(&grid_sys_state, grid_sys_get_bank_number_of_first_valid(&grid_sys_state));
				
				grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_CFG_RESPONSE);
				
				
				uint8_t heartbeateventnum = grid_ui_event_find(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_HEARTBEAT);

				if (heartbeateventnum != 255){
					char* actionstring = grid_core_state.bank_list[0].element_list[0].event_list[heartbeateventnum].action_string;
			
			
					grid_msg_set_parameter(actionstring, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, 1, NULL);

				}



				usb_init_flag = 1;
		
			}
			
		}
		

		// Request neighbor bank settings if we don't have it initialized
		
 		if (grid_sys_get_bank_valid(&grid_sys_state) == 0 && loopcounter%80 == 0){
 								
			if (grid_sys_state.bank_init_flag == 0)	{
				
				grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_CFG_REQUEST);
			}				 		
			 
 		}

		
		loopcounter++;

		if (loopcounter == 1000){


			printf("vTaskStartScheduler! \r\n");
			delay_ms(2);
			//vTaskStartScheduler();

		}


		//gpio_set_pin_level(MUX_A, true);

		
		int loopc = loopcounter;


		
		if (sys_i2c_enabled){

			sys_i2c_done_flag = SYS_I2C_STATUS_BUSY;



			uint8_t buffer[2] = {0x01, 0x00};

			if (1){
				
				i2c_m_async_set_slaveaddr(&SYS_I2C, 0x22, I2C_M_SEVEN);

				struct io_descriptor *const io = SYS_I2C_io;
				const uint8_t *buf = buffer;
				const uint16_t n = 1;

				if (1){


					struct i2c_m_async_desc *i2c = &SYS_I2C;
					struct _i2c_m_msg        msg;
					int32_t                  ret;

					msg.addr   = i2c->slave_addr;
					msg.len    = n;
					msg.flags  = 0;
					msg.buffer = (uint8_t *)buf;

					/* start transfer then return */
					ret = i2c_m_async_transfer(&i2c->device, &msg);

					if (ret != 0) {
						printf("I2C error\r\n");
					}

					uint32_t cycles = grid_d51_dwt_cycles_read();


					while (i2c->device.service.msg.flags & I2C_M_BUSY) {
						;
					}

					msg.flags  = I2C_M_RD | I2C_M_STOP;		
					ret = i2c_m_async_transfer(&i2c->device, &msg);

					if (ret != 0) {
						printf("I2C error\r\n");
					}

					while (i2c->device.service.msg.flags & I2C_M_BUSY) {
						;
					}



					printf("I2C: %d ,Elapsed %dus\r\n", buf[0], (grid_d51_dwt_cycles_read()-cycles)/120);

					//return (int32_t)n;
				}
				else{

					io_write(SYS_I2C_io, SYS_I2C_example_str, 10); //works
				}


				//printf("Buffer: %d\r\n", buffer);
			}

			uint8_t* txbuffer[4] = {0}; 
			uint8_t* rxbuffer[10] = {0}; 

			txbuffer[0] = 0x01; // ID

		}

		usb_task_inner();
	
		nvm_task_inner();
		
		receive_task_inner();

		ui_task_inner();
	
		inbound_task_inner();

		outbound_task_inner();

		led_task_inner();

		lua_gc(grid_lua_state.L, LUA_GCCOLLECT);

	}//WHILE
	
	
	
}//MAIN

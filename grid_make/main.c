

#include "grid/grid_module.h"

#include "grid/grid_ui.h"

#include <atmel_start.h>
#include "atmel_start_pins.h"

#include <hal_qspi_dma.h>


#include <stdio.h>


#include <hpl_reset.h>


#include "hal_rtos.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"



#include "usb/class/midi/device/audiodf_midi.h"


void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName ){

	CRITICAL_SECTION_ENTER()

	while(1){

		printf("Stack overflow in task %s\r\n", pcTaskGetName(xTask));
		delay_ms(1000);
	}

	CRITICAL_SECTION_LEAVE()


}

static TaskHandle_t      		xCreatedUiTask;
#define TASK_UI_STACK_SIZE 		(4*1024 / sizeof(portSTACK_TYPE))
#define TASK_UI_PRIORITY 		(5)

static TaskHandle_t      		xCreatedUsbTask;
#define TASK_USB_STACK_SIZE 	(2048 / sizeof(portSTACK_TYPE))
#define TASK_USB_PRIORITY 		(1)

static TaskHandle_t      		xCreatedNvmTask;
#define TASK_NVM_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_NVM_PRIORITY 		(2)

static TaskHandle_t      			xCreatedReceiveTask;
#define TASK_RECEIVE_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_RECEIVE_PRIORITY 		(2)

static TaskHandle_t      			xCreatedInboundTask;
#define TASK_INBOUND_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_INBOUND_PRIORITY 		(2)

static TaskHandle_t      			xCreatedOutboundTask;
#define TASK_OUTBOUND_STACK_SIZE 	(1024 / sizeof(portSTACK_TYPE))
#define TASK_OUTBOUND_PRIORITY 		(2)

static TaskHandle_t      		xCreatedLedTask;
#define TASK_LED_STACK_SIZE 	(16*1024 / sizeof(portSTACK_TYPE))
#define TASK_LED_PRIORITY 		(4)


#define TASK_EXAMPLE_STACK_SIZE (512 / sizeof(portSTACK_TYPE))
#define TASK_EXAMPLE_STACK_PRIORITY (tskIDLE_PRIORITY + 2)
#define TASK_EXAMPLE2_STACK_PRIORITY (tskIDLE_PRIORITY + 3)

static TaskHandle_t      xCreatedExampleTask;
static TaskHandle_t      xCreatedExample2Task;

static SemaphoreHandle_t disp_mutex;

/**
 * OS example task
 *
 * \param[in] p The void pointer for OS task Standard model.
 *
 */

volatile uint32_t globaltest = 0;

static void example_task(void *p)
{
	(void)p;
	while (1) {

		// if (xSemaphoreTake(disp_mutex, 1)) {
		// 	/* add your code */

		// 	printf("ExampleTask ????... %d \r\n", globaltest);

		// }

		// if (globaltest%4 == 0){


		// 	xSemaphoreGive(disp_mutex);

		// }
		CRITICAL_SECTION_ENTER()

		printf("ExampleTask ????... %d \r\n", globaltest);
		CRITICAL_SECTION_LEAVE()


		vTaskDelay(1000*configTICK_RATE_HZ/1000);

		//os_sleep(400);

	}
}


static void example2_task(void *p)
{
	(void)p;
	while (1) {

		//printf("Example2 Task Printing... \r\n");
		//globaltest++;
		vTaskDelay(1000); //  portTICK_PERIOD_MS


	}
}


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

		ui_task_inner();



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



void RTC_Scheduler_realtime_cb(const struct timer_task *const timer_task)
{








	grid_sys_rtc_tick_time(&grid_sys_state);	
	grid_task_timer_tick(&grid_task_state);
			
	uint8_t mapmode_value = !gpio_get_pin_level(MAP_MODE);

	if (mapmode_value != grid_sys_state.mapmodestate){
		
		grid_sys_state.mapmodestate = mapmode_value;
			
		if (grid_sys_state.mapmodestate == 0){ // RELEASE
			
			grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_MAPMODE_RELEASE);
			

				
		}
		else{ // PRESS
			
			grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_MAPMODE_PRESS);		
			


									 
		}

	}

}

void RTC_Scheduler_heartbeat_cb(const struct timer_task *const timer_task)
{
	
	grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_HEARTBEAT);

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

int main(void)
{


	

	// boundary scan here

	uint32_t boundary_result[4] = {0};

	grid_d51_boundary_scan(boundary_result);



	atmel_start_init();	
    
            
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_PORT, "Start Initialized");


	for (uint8_t i=0; i<4; i++){

		printf("boundary_result[%d] = ", i);

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

	uint8_t test_string[] = "print(if(10>20,(1+2),(3+4)),4)";

	//grid_expr_evaluate(&grid_expr_state, test_string, strlen(test_string));
	//NVIC_SystemReset();


	grid_module_common_init();
    grid_ui_reinit(&grid_ui_state);
	
			
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Grid Module Initialized");

	init_timer();
	
	uint32_t loopcounter = 0;
	
	uint32_t loopstart = 0;
	
	uint32_t loopslow = 0;
	uint32_t loopfast = 0;
	uint32_t loopwarp = 0;
	
	uint8_t usb_init_flag = 0;	

	
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Entering Main Loop");
	//GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Entering Main Loop2 test");
	

	// Init Bank Color Bug when config was previously saved
	
	grid_sys_nvm_load_configuration(&grid_sys_state, &grid_nvm_state);
	grid_ui_nvm_load_all_configuration(&grid_ui_state, &grid_nvm_state);	
	

//     Clear NVM if user is stuc with old incompatible config	
//    grid_ui_nvm_clear_all_configuration(&grid_ui_state, &grid_nvm_state);	
//    while (grid_nvm_ui_bulk_clear_is_in_progress(&grid_nvm_state, &grid_ui_state)){
//
//        grid_nvm_ui_bulk_clear_next(&grid_nvm_state, &grid_ui_state);
//
//
//    }

	// disp_mutex = xSemaphoreCreateMutex();

	// if (disp_mutex == NULL) {
	// 	while (1) {
	// 		;
	// 	}
	// }


    
	if (xTaskCreate(usb_task, "Usb Task", TASK_USB_STACK_SIZE, NULL, TASK_USB_PRIORITY, &xCreatedUsbTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}

	if (xTaskCreate(usb_task, "Nvm Task", TASK_NVM_STACK_SIZE, NULL, TASK_NVM_PRIORITY, &xCreatedNvmTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}

	if (xTaskCreate(usb_task, "Ui Task", TASK_UI_STACK_SIZE, NULL, TASK_UI_PRIORITY, &xCreatedUiTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}

	if (xTaskCreate(receive_task, "Receive Task", TASK_RECEIVE_STACK_SIZE, NULL, TASK_RECEIVE_PRIORITY, &xCreatedReceiveTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}

	if (xTaskCreate(inbound_task, "Inbound Task", TASK_INBOUND_STACK_SIZE, NULL, TASK_INBOUND_PRIORITY, &xCreatedInboundTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}

	if (xTaskCreate(outbound_task, "Outbound Task", TASK_OUTBOUND_STACK_SIZE, NULL, TASK_OUTBOUND_PRIORITY, &xCreatedOutboundTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}


	if (xTaskCreate(led_task, "Led Task", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, &xCreatedLedTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}
    
	if (xTaskCreate(example_task, "Example", TASK_EXAMPLE_STACK_SIZE, NULL, TASK_EXAMPLE_STACK_PRIORITY, &xCreatedExampleTask)
	    != pdPASS) {
		while (1) {
			;
		}
	}
	
	while (1) {
	
		
		if (usb_init_flag == 0){
			
			
			if (usb_d_get_frame_num() == 0){
				
			}
			else{			
			
				grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN	
				
				GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Composite Device Connected");
				
				grid_sys_set_bank(&grid_sys_state, grid_sys_get_bank_number_of_first_valid(&grid_sys_state));
				
				grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_CFG_RESPONSE);
				
				
				usb_init_flag = 1;
				
				

				printf("Forever! \r\n");
				delay_ms(2);


				vTaskStartScheduler();

				while(1){
				
		
					
					usb_task_inner();
					nvm_task_inner();

					receive_task_inner();

					ui_task_inner();

					inbound_task_inner();

					outbound_task_inner();

					led_task_inner();

					delay_ms(1);
				}
			}
			
		}
		
		
		
		// Request neighbor bank settings if we don't have it initialized
		
 		if (grid_sys_get_bank_valid(&grid_sys_state) == 0 && loopcounter%80 == 0){
 								
			if (grid_sys_state.bank_init_flag == 0)	{
				
				grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_CFG_REQUEST);
			}				 		
			 
 		}

		
		loopcounter++;
	

		
		
		
		usb_task_inner();
		


		nvm_task_inner();
		
					


		receive_task_inner();

		ui_task_inner();
		
		
		
		
		
		inbound_task_inner();

		outbound_task_inner();

		led_task_inner();

		delay_ms(1);

	}//WHILE
	
	
	
}//MAIN

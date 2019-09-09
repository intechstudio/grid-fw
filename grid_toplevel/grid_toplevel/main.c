#include <atmel_start.h>
#include "atmel_start_pins.h"

#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry
#include "../../grid_lib/grid_sys.c" // Grid System

#define GRID_MODULE_P16

#include "../../grid_modules/grid_module_p16.c" // 


volatile uint8_t task1flag = 0;
volatile uint8_t task2flag = 0;

volatile uint8_t reportflag = 0;


static struct timer_task RTC_Scheduler_tick;
static struct timer_task RTC_Scheduler_report;
static struct timer_task RTC_Scheduler_task2;



#define TASK_UNDEFINED 0
#define TASK_IDLE	   1
#define TASK_LED   2
#define TASK_UIIN    3

volatile uint8_t task_current = 0;
volatile uint32_t task_counter[8] = {0, 0, 0, 0, 0, 0, 0, 0 };



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

#define RTC1SEC 16384

void init_timer(void)
{
	RTC_Scheduler_tick.interval = 1;
	RTC_Scheduler_tick.cb       = RTC_Scheduler_tick_cb;
	RTC_Scheduler_tick.mode     = TIMER_TASK_REPEAT;
	
	
	RTC_Scheduler_report.interval = 32768/2; //1sec
	RTC_Scheduler_report.cb       = RTC_Scheduler_report_cb;
	RTC_Scheduler_report.mode     = TIMER_TASK_REPEAT;
	
	RTC_Scheduler_task2.interval = 32768/2*20;
	RTC_Scheduler_task2.cb       = RTC_Scheduler_task2_cb;
	RTC_Scheduler_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_tick);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_task2);
	timer_start(&RTC_Scheduler);
}

int main(void)
{

	
	atmel_start_init();
	
	//cdcd_acm_example();
	
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
	


	while (1) {
		
		//checktimer flags
		if (reportflag){
			
			
			char str[70];
			sprintf(str, "LOOPTICK %d\nREALTIME %d\nTASK0 %d\nTASK1 %d\nTASK2 %d\nTASK3 %d\n\0", loopcounter, realtime, task_counter[0], task_counter[1], task_counter[2], task_counter[3] );
			cdcdf_acm_write(str, strlen(str));
			
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
			
			
			char str[11];
			sprintf(str, "TASKTICK 2\n");

			
			cdcdf_acm_write(str, 11);
			
			task2flag--;
			
			
		}		
		
			
		loopstart = realtime;
		
		
		if (mapmode != gpio_get_pin_level(MAP_MODE)){
			
			
			if (mapmode==0){
				sysmode = ! sysmode;
			}
			
			mapmode = !mapmode;
			/* ==================== Reading MCU Unique Serial Nuber ====================== */
			
			uint32_t id_array[4];
			grid_sys_get_id(id_array);
			
			/* ========================= Reading the HWCFG ROM =========================== */

			uint32_t hwcfg = grid_sys_get_hwcfg();
			
			// REPORT OVER SERIAL
			
			struct io_descriptor *io_uart_aux;
			
			usart_async_get_io_descriptor(&GRID_AUX, &io_uart_aux);
			usart_async_enable(&GRID_AUX);

			char example_GRID_AUX[60];
			sprintf(example_GRID_AUX, "HWCFG: %08x\nUNIQUE: %08x %08x %08x %08x     \n", hwcfg, id_array[0], id_array[1], id_array[2], id_array[3]);

			io_write(io_uart_aux, example_GRID_AUX, 65);
			
			// USB CDC SERIAL AS DEBUG PORT
			//cdcdf_acm_write(example_GRID_AUX, 65);
			
			char str[20];
			sprintf(str, "HEXTEST %x\n\0", loopcounter);
			cdcdf_acm_write(str, strlen(str));
			
		}
		
		
		
		if (faketimer > 100){
			grid_tel_frequency_tick();
			faketimer = 0;
		}
		faketimer++;
		
			
		/* ========================= ANALOG READ ============================= */
		
		


		
		// Push out all changes
		
		task_current = TASK_UIIN;
		
		for (uint8_t i = 0; i<16; i++)
		{
			if (grid_ain_get_changed(i)){
				
				grid_tel_event_handler(console_tx);
				
				uint16_t average = grid_ain_get_average(i);
				
				
				/*
				
				
				char str[26];
				sprintf(str, "ADC: %5d %5d %5d \n", i, average, average/128);

				//USART
				io_write(io, str, 24);
				
								
				uint32_t crc = 0xFFFFFFFF;
 				crc_sync_crc32(&CRC_0, (uint32_t *)str, 5, &crc);
 				crc ^= 0xFFFFFFFF;
				
							
				
				char str2[26];
				sprintf(str2, "CRC: %x \n", crc);
				//USART
				//	io_write(io, str2, 15);
				*/	

				char str3[50];
			
				sprintf(str3, "AIN%d %d\n\0", i, average/64);	
				cdcdf_acm_write(str3, strlen(str3));
		

					
				
							
				grid_led_set_phase(i, 0, average*2/128); // 0...255
				
				
			}
			
		}
		
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

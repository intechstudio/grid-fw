/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"

#include <string.h>

#include "driver/spi_master.h"
#include "rom/ets_sys.h" // For ets_printf

#include "driver/gpio.h"
#include "esp_flash.h"
#include "esp_task_wdt.h"


#include "pico_firmware.h"
#include "grid_esp32_module_pbf4.h"
#include "grid_esp32_module_po16.h"
#include "grid_esp32_module_bu16.h"
#include "grid_esp32_module_en16.h"
#include "grid_esp32_module_ef44.h"
#include "grid_esp32_led.h"


#define ADC_TASK_PRIORITY 4
#define LED_TASK_PRIORITY 5

// NVM must not be preemted by Port task
#define NVM_TASK_PRIORITY 1
#define PORT_TASK_PRIORITY 3


#define USB_TASK_PRIORITY 3

#include "driver/ledc.h"
#include <esp_timer.h>

#include "grid_esp32.h"
#include "grid_esp32_swd.h"
#include "grid_esp32_port.h"
#include "grid_esp32_nvm.h"
#include "grid_esp32_usb.h"


#include "esp_log.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf


#include "../../grid_common/grid_protocol.h"
#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_led.h"
#include "../../grid_common/grid_sys.h"
#include "../../grid_common/grid_msg.h"
#include "../../grid_common/grid_buf.h"
#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_usb.h"
#include "../../grid_common/grid_module.h"

#include "../../grid_common/grid_lua_api.h"
#include "../../grid_common/grid_ui.h"


#include "../../grid_common/lua-5.4.3/src/lua.h"
#include "../../grid_common/lua-5.4.3/src/lualib.h"
#include "../../grid_common/lua-5.4.3/src/lauxlib.h"


static const char *TAG = "main";


#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static void midi_task_read_example(void *arg)
{
    // The MIDI interface always creates input and output port/jack descriptors
    // regardless of these being used or not. Therefore incoming traffic should be read
    // (possibly just discarded) to avoid the sender blocking in IO
    uint8_t packet[4];
    bool read = false;
    for (;;) {
        vTaskDelay(1);
        while (tud_midi_available()) {
            read = tud_midi_packet_read(packet);
            if (read) {
                ESP_LOGI(TAG, "Read - Time (ms since boot): %lld, Data: %02hhX %02hhX %02hhX %02hhX",
                         esp_timer_get_time(), packet[0], packet[1], packet[2], packet[3]);
            }
        }
    }
}



static void periodic_rtc_ms_cb(void *arg)
{

    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);
    grid_ui_rtc_ms_tick_time(&grid_ui_state);	


    if (gpio_get_level(GRID_ESP32_PINS_MAPMODE)){
        grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 0);
    }
    else{
        grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 1);
    }
		

}



void system_init_core_2_task(void *arg)
{


    grid_esp32_swd_pico_pins_init(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, GRID_ESP32_PINS_RP_CLOCK);
    grid_esp32_swd_pico_clock_init(LEDC_TIMER_0, LEDC_CHANNEL_0);
    grid_esp32_swd_pico_program_sram(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, pico_firmware, pico_firmware_len);

    vTaskSuspend(NULL);
}


void app_main(void)
{



    ESP_LOGI(TAG, "===== MAIN START =====");

    gpio_set_direction(GRID_ESP32_PINS_MAPMODE, GPIO_MODE_INPUT);
    gpio_pullup_en(GRID_ESP32_PINS_MAPMODE);



    grid_esp32_usb_init();
    grid_usb_midi_buffer_init();
    grid_usb_keyboard_buffer_init(&grid_keyboard_state);

    TaskHandle_t core2_task_hdl;
    xTaskCreatePinnedToCore(system_init_core_2_task, "swd_init", 1024*3, NULL, 4, &core2_task_hdl, 1);


    // GRID MODULE INITIALIZATION SEQUENCE


    ESP_LOGI(TAG, "===== NVM START =====");
    grid_esp32_nvm_init(&grid_esp32_nvm_state);

    ESP_LOGI(TAG, "===== SYS START =====");
    grid_sys_init(&grid_sys_state);

    ESP_LOGI(TAG, "===== MSG START =====");
	grid_msg_init(&grid_msg_state); //setup session id, last message buffer init




    ESP_LOGI(TAG, "===== LUA INIT =====");
	grid_lua_init(&grid_lua_state);

    // ================== START: grid_module_pbf4_init() ================== //

	
    ESP_LOGI(TAG, "===== PORT INIT =====");
    grid_port_init_all(); // buffers
    
    ESP_LOGI(TAG, "===== BANK INIT =====");
    grid_sys_set_bank(&grid_sys_state, 0);

    ESP_LOGI(TAG, "===== UI INIT =====");
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevD){
		grid_module_po16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevD ){
		grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevD){
        grid_module_pbf4_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD ){
		grid_module_en16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ){
		grid_module_en16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}		
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD ){
		grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);		
	}	
	else{
		ets_printf("Init Module: Unknown Module\r\n");
	}




    grid_ui_page_load(&grid_ui_state, 0); //load page 0

    while (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state))
    {
        grid_ui_bulk_pageread_next(&grid_ui_state);
    }
        


    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

    

    ESP_LOGI(TAG, "===== UI TASK INIT =====");

    TaskHandle_t module_task_hdl;
	if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevD){
        xTaskCreatePinnedToCore(grid_esp32_module_po16_task, "po16", 1024*3, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevD ){
        xTaskCreatePinnedToCore(grid_esp32_module_bu16_task, "bu16", 1024*3, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevD){
        xTaskCreatePinnedToCore(grid_esp32_module_pbf4_task, "pbf4", 1024*3, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD ){
        xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024*2, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}	
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ){
        xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024*2, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}		
	else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD ){
        xTaskCreatePinnedToCore(grid_esp32_module_ef44_task, "ef44", 1024*4, (void *)signaling_sem, ADC_TASK_PRIORITY, &module_task_hdl, 0);
	}	
	else{
		printf("Init Module: Unknown Module\r\n");
	}


    ESP_LOGI(TAG, "===== UI TASK DONE =====");
    


    grid_ui_state.ui_interaction_enabled = 1;
    // ================== FINISH: grid_module_pbf4_init() ================== //



    



    //Create the class driver task
    TaskHandle_t led_task_hdl;
    xTaskCreatePinnedToCore(grid_esp32_led_task,
                            "led",
                            1024*3,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);

    TaskHandle_t nvm_task_hdl;


    TaskHandle_t port_task_hdl;
    SemaphoreHandle_t nvm_or_port = xSemaphoreCreateBinary();
    xSemaphoreGive(nvm_or_port);


    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_port_task,
                            "port",
                            4096*10,
                            (void *)nvm_or_port,
                            PORT_TASK_PRIORITY,
                            &port_task_hdl,
                            0);




    
    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_nvm_task,
                            "nvm",
                            1024*5,
                            (void *)nvm_or_port,
                            NVM_TASK_PRIORITY,
                            &nvm_task_hdl,
                            0);


    TaskHandle_t housekeeping_task_hdl;

    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_housekeeping_task,
                            "housekeeping",
                            1024*5,
                            (void *)signaling_sem,
                            6,
                            &housekeeping_task_hdl,
                            0);




    esp_timer_create_args_t periodic_rtc_ms_args = {
        .callback = &periodic_rtc_ms_cb,
        .name = "rtc millisecond"
    };

   esp_timer_handle_t periodic_rtc_ms_timer;
   ESP_ERROR_CHECK(esp_timer_create(&periodic_rtc_ms_args, &periodic_rtc_ms_timer));
   ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_rtc_ms_timer, 10000));





    


#if CFG_TUD_MIDI

    // Read recieved MIDI packets
    ESP_LOGI(TAG, "MIDI read task init");
    xTaskCreatePinnedToCore(midi_task_read_example, "midi_task_read_example", 2 * 1024, NULL, 5, NULL, 0);
#endif

    

    ESP_LOGI(TAG, "===== INIT COMPLETE =====");

}

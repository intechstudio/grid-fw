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
#include "grid_esp32_adc.h"
#include "grid_esp32_led.h"


#define LED_TASK_PRIORITY 2
#define LED_TASK_PRIORITY 2


#include "driver/ledc.h"
#include <esp_timer.h>

#include "grid_esp32.h"
#include "grid_esp32_swd.h"

#include "esp_log.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf


#include "../../grid_common/grid_protocol.h"
#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_led.h"


void app_main(void)
{

    static const char *TAG = "main";

    grid_esp32_get_hwcfg();
    grid_esp32_get_cpuid();

    // GRID_MODULE_INIT (based on hwcfg)


    ets_printf("LED INIT ...\r\n");

    grid_led_init(&grid_led_state, 16);

    ets_printf("LED INIT DONE\r\n");


    vTaskDelay(100);
    
    grid_esp32_swd_pico_pins_init(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, GRID_ESP32_PINS_RP_CLOCK);
    grid_esp32_swd_pico_clock_init(LEDC_TIMER_0, LEDC_CHANNEL_0);
    grid_esp32_swd_pico_program_sram(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, ___grid_pico_build_main_main_bin, ___grid_pico_build_main_main_bin_len);


    ets_printf("Testing External Libraries:\r\n");
    ets_printf("Version: %d %d %d\r\n", GRID_PROTOCOL_VERSION_MAJOR, GRID_PROTOCOL_VERSION_MINOR, GRID_PROTOCOL_VERSION_PATCH );
    ets_printf("grid_ain_abs test -7 -> %d\r\n", grid_ain_abs(-7));


    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();



    TaskHandle_t adc_task_hdl;


    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_adc_task,
                            "adc",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &adc_task_hdl,
                            0);


    TaskHandle_t led_task_hdl;

    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_led_task,
                            "led",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);



}

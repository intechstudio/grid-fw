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


#include "driver/ledc.h"


#define LED_TASK_PRIORITY 2
extern void led_task(void *arg);


#include <esp_timer.h>

#include "grid_esp32.h"
#include "grid_esp32_swd.h"






void app_main(void)
{

    static const char *TAG = "main";

    grid_esp32_get_hwcfg();
    grid_esp32_get_cpuid();


    vTaskDelay(100);
    
    grid_esp32_swd_init_coprocessor();



    ledc_timer_config_t pwm_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 12000000,
        .duty_resolution = LEDC_TIMER_2_BIT,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&pwm_config));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GRID_ESP32_PINS_RP_CLOCK,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));


esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);
esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel);





    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();



    TaskHandle_t led_task_hdl;

    //Create the class driver task
    xTaskCreatePinnedToCore(led_task,
                            "led",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);


}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_port.h"


static const char *TAG = "PORT";


void grid_esp32_port_task(void *arg)
{


    ESP_LOGI(TAG, "Initializing all ports!");
    grid_port_init_all();

    static uint32_t loopcounter = 0;

    while (1) {

        loopcounter++;


        ESP_LOGI(TAG, "Ping!");

        grid_protocol_send_heartbeat(); // Put ping into UI rx_buffer
        grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback , put rx_buffer content to each port's tx_buffer

        
        grid_port_process_outbound_usb(&GRID_PORT_H);


        //grid_platform_usb_serial_write(TAG, strlen(TAG));

        vTaskDelay(pdMS_TO_TICKS(250));



    }


    ESP_LOGI(TAG, "Deinit PORT");
    vTaskSuspend(NULL);
}

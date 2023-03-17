/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_port.h"


static const char *TAG = "PORT";

#define GPIO_MOSI 8
#define GPIO_MISO 6
#define GPIO_SCLK 9
#define GPIO_CS 7
#define RCV_HOST    SPI2_HOST


//Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
void my_post_setup_cb(spi_slave_transaction_t *trans) {
    //printf("$\r\n");
}

//Called after transaction is sent/received. We use this to set the handshake line low.
void my_post_trans_cb(spi_slave_transaction_t *trans) {
    
   // printf("@\r\n");
}


void grid_esp32_port_task(void *arg)
{

    int n=0;
    esp_err_t ret;

    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=GPIO_MOSI,
        .miso_io_num=GPIO_MISO,
        .sclk_io_num=GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        .mode=0,
        .spics_io_num=GPIO_CS,
        .queue_size=3,
        .flags=0,
        .post_setup_cb=my_post_setup_cb,
        .post_trans_cb=my_post_trans_cb
    };


    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    ret=spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret==ESP_OK);

    WORD_ALIGNED_ATTR char sendbuf[512+1]={0};
    WORD_ALIGNED_ATTR char recvbuf[512+1]={0};
    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));


    static uint32_t loopcounter = 0;

    while (1) {

        //Clear receive buffer, set send buffer to something sane
        memset(recvbuf, 0xA5, 512+1);        
        sprintf(sendbuf, "This is the receiver, sending data for transmission number %04d.", n);

        loopcounter++;

        //Set up a transaction of 512 bytes to send/receive
        t.length=512*8;
        t.tx_buffer=sendbuf;
        t.rx_buffer=recvbuf;
        /* This call enables the SPI slave interface to send/receive to the sendbuf and recvbuf. The transaction is
        initialized by the SPI master, however, so it will not actually happen until the master starts a hardware transaction
        by pulling CS low and pulsing the clock etc. In this specific example, we use the handshake line, pulled up by the
        .post_setup_cb callback that is called as soon as a transaction is ready, to let the master know it is free to transfer
        data.
        */
        ret=spi_slave_transmit(RCV_HOST, &t, portMAX_DELAY);
        //spi_slave_queue_trans(RCV_HOST, &t, 0);

        //spi_slave_transmit does not return until the master has done a transmission, so by here we have sent our data and
        //received data from the master. Print it.
        printf("Received: %s\n", recvbuf);
        n++;


	
        if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && tud_connected()){
        
            printf("USB CONNECTED\r\n\r\n");
            printf("HWCFG %ld\r\n", grid_sys_get_hwcfg(&grid_sys_state));

            grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 100);	
            grid_led_set_alert_frequency(&grid_led_state, -2);	
            grid_led_set_alert_phase(&grid_led_state, 200);	
            
            grid_msg_set_heartbeat_type(&grid_msg_state, 1);

    
        }
			




        //ESP_LOGI(TAG, "Ping!");
        if (loopcounter%30 == 0){

            grid_protocol_send_heartbeat(); // Put ping into UI rx_buffer
        }


        if (loopcounter%4 == 0){

            if (grid_ui_event_count_istriggered_local(&grid_ui_state)){

                //CRITICAL_SECTION_ENTER()
                grid_port_process_ui_local_UNSAFE(&grid_ui_state);
                //CRITICAL_SECTION_LEAVE()
            
            }
			if (grid_ui_event_count_istriggered(&grid_ui_state)){

				grid_ui_state.port->cooldown += 3;	

				//CRITICAL_SECTION_ENTER()
				grid_port_process_ui_UNSAFE(&grid_ui_state); 
				//CRITICAL_SECTION_LEAVE()
			}


        }



	    grid_port_receive_task(&GRID_PORT_H); // USB
	    grid_port_receive_task(&GRID_PORT_U); // UI
        
        // INBOUND
        grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback , put rx_buffer content to each CONNECTED port's tx_buffer
        // ... GRID UART PORTS ...
        grid_port_process_inbound(&GRID_PORT_H, 0);





        // OUTBOUND
        // ... GRID UART PORTS ...
        grid_port_process_outbound_usb(&GRID_PORT_H);       
        grid_port_process_outbound_ui(&GRID_PORT_U);

        //GRID_PORT_U.rx_buffer.read_start = 0;        
        //GRID_PORT_U.rx_buffer.read_stop = 0;  

    

        //GRID_PORT_H.rx_buffer.read_start = 0;        
        //GRID_PORT_H.rx_buffer.write_start = 0;  

        //grid_platform_usb_serial_write(TAG, strlen(TAG));

        vTaskDelay(pdMS_TO_TICKS(10));



    }


    ESP_LOGI(TAG, "Deinit PORT");
    vTaskSuspend(NULL);
}

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


spi_slave_transaction_t t;


spi_slave_transaction_t spi_empty_transaction;

uint8_t queue_state = 0;

//Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
static void IRAM_ATTR my_post_setup_cb(spi_slave_transaction_t *trans) {
    //printf("$\r\n");
}

uint8_t spi_ready = 1;

//Called after transaction is sent/received. We use this to set the handshake line low.
static void IRAM_ATTR  my_post_trans_cb(spi_slave_transaction_t *trans) {

    if (queue_state>0){
        queue_state--;
    }   


    spi_ready = 1;

    // ets_printf("SPI[499] = %d\r\n", ((uint8_t*) trans->tx_buffer)[499]);
    //ets_printf("SPI[499] = %d\r\n", ((uint8_t*) trans->rx_buffer)[499]);

    uint8_t ready_flags = ((uint8_t*) trans->rx_buffer)[499];



    if ((ready_flags&0b00000001)){
        GRID_PORT_N.tx_double_buffer_status = 0;
    }

    if ((ready_flags&0b00000010)){
        GRID_PORT_E.tx_double_buffer_status = 0;
    }

    if ((ready_flags&0b00000100)){
        GRID_PORT_S.tx_double_buffer_status = 0;
    }

    if ((ready_flags&0b00001000)){
        GRID_PORT_W.tx_double_buffer_status = 0;
    }   


    spi_slave_transaction_t *result;




    //spi_slave_get_trans_result(RCV_HOST, &result, 0);
    //grid_platform_printf("@ SPI COMPLETE: ");

}


uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length){

    t.tx_buffer = GRID_PORT_N.tx_double_buffer;    
    GRID_PORT_N.tx_double_buffer[GRID_DOUBLE_BUFFER_TX_SIZE-1] = 1;

    spi_slave_queue_trans(RCV_HOST, &t, portMAX_DELAY);
    queue_state++;

    spi_ready = 0;


    
    //ets_printf("SPI queued %s %d, queue_state = %d\r\n", GRID_PORT_N.tx_double_buffer,  GRID_PORT_N.tx_double_buffer_status, queue_state);



    return 0; // done

}


void grid_esp32_port_task(void *arg)
{

    uint8_t n=0;
    esp_err_t ret;

    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=GPIO_MOSI,
        .miso_io_num=GPIO_MISO,
        .sclk_io_num=GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .intr_flags = ESP_INTR_FLAG_IRAM
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        .mode=0,
        .spics_io_num=GPIO_CS,
        .queue_size=5,
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
    
    memset(&t, 0, sizeof(t));

    //Clear receive buffer, set send buffer to something sane
    memset(recvbuf, 0xA5, 512+1);        
    sprintf(sendbuf, "This is the receiver, sending data for transmission number %04d.", n);


    //Set up a transaction of 512 bytes to send/receive
    t.length=512*8;
    t.tx_buffer=sendbuf;
    t.rx_buffer=recvbuf;

    uint8_t empty_tx_buffer[512] = {0};

    empty_tx_buffer[0] = 'X';

    spi_empty_transaction.length=512*8;
    spi_empty_transaction.tx_buffer=empty_tx_buffer;
    spi_empty_transaction.rx_buffer=recvbuf;


    GRID_PORT_N.partner_status = 1; // force connected

    static uint32_t loopcounter = 0;
    //ret=spi_slave_queue_trans(RCV_HOST, &t, 0);
    ret=spi_slave_queue_trans(RCV_HOST, &t, portMAX_DELAY);
    queue_state++;
    //spi_ready = 0;


    uint8_t pingcounter = 0;

    while (1) {


        if (queue_state == 0){
            spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, portMAX_DELAY);
            queue_state++;
        }


        //printf("LOOP %d\r\n", queue_state);

        pingcounter++;
        if (pingcounter%10 == 0){
            //ets_printf("TRY PING\r\n");

            GRID_PORT_N.ping_flag = 1;

            grid_port_ping_try_everywhere();
            
        }

        
        grid_port_process_outbound_usart(&GRID_PORT_N);

        if (spi_ready == 1){



            n++;
            sprintf(sendbuf, "This is the receiver, sending data for transmission number %04d.", n);

           // grid_platform_printf("@ READY COMPLETE: ");
            spi_slave_transaction_t *trans = NULL;
            spi_slave_get_trans_result(RCV_HOST, &trans, portMAX_DELAY);

            ets_printf("RX %d: %s\r\n", ((uint8_t*) trans->rx_buffer)[499], ((uint8_t*) trans->rx_buffer));
          //  grid_platform_printf("@%d: %s %s\r\n", trans->length, trans->tx_buffer, trans->rx_buffer);

            
            //spi_slave_queue_trans(RCV_HOST, &t, 0);
        }

        loopcounter++;

	
        if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && tud_connected()){
        
            printf("USB CONNECTED\r\n\r\n");
            printf("HWCFG %ld\r\n", grid_sys_get_hwcfg(&grid_sys_state));

            grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 100);	
            grid_led_set_alert_frequency(&grid_led_state, -2);	
            grid_led_set_alert_phase(&grid_led_state, 200);	
            
            grid_msg_set_heartbeat_type(&grid_msg_state, 1);

    
        }

        //ESP_LOGI(TAG, "Ping!");
        if (loopcounter%32 == 0){
            vTaskSuspendAll();
            grid_protocol_send_heartbeat(); // Put ping into UI rx_buffer
            xTaskResumeAll();
        }


        if (loopcounter%4 == 0){

            if (grid_ui_event_count_istriggered_local(&grid_ui_state)){

                //CRITICAL_SECTION_ENTER()
                vTaskSuspendAll();
                grid_port_process_ui_local_UNSAFE(&grid_ui_state);
                xTaskResumeAll();
                //CRITICAL_SECTION_LEAVE()
            
            }
			if (grid_ui_event_count_istriggered(&grid_ui_state)){

				grid_ui_state.port->cooldown += 3;	

				//CRITICAL_SECTION_ENTER()
                vTaskSuspendAll();
				grid_port_process_ui_UNSAFE(&grid_ui_state); 
                xTaskResumeAll();
				//CRITICAL_SECTION_LEAVE()
			}


        }



	    grid_port_receive_task(&GRID_PORT_H); // USB
	    grid_port_receive_task(&GRID_PORT_U); // UI
        


        // INBOUND

        vTaskSuspendAll();
        grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback , put rx_buffer content to each CONNECTED port's tx_buffer
        xTaskResumeAll();
        
        
        
        // ... GRID UART PORTS ...
        
        vTaskSuspendAll();
        grid_port_process_inbound(&GRID_PORT_H, 0);
        xTaskResumeAll();


        //grid_port_process_inbound(&GRID_PORT_N, 0);



        grid_port_process_inbound(&GRID_PORT_N, 0);


        // OUTBOUND
        // ... GRID UART PORTS ...
        
        //vTaskSuspendAll();
        grid_port_process_outbound_usb(&GRID_PORT_H); 
        //xTaskResumeAll();


        //vTaskSuspendAll();        
        grid_port_process_outbound_ui(&GRID_PORT_U);
        //xTaskResumeAll();



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

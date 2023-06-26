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

uint8_t empty_tx_buffer[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};
uint8_t message_tx_buffer[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

spi_slave_transaction_t outbnound_transaction[4];
spi_slave_transaction_t spi_empty_transaction;

uint8_t queue_state = 0;


void ets_debug_string(char* tag, char* str){

    return;

    uint16_t length = strlen(str);

    ets_printf("%s: ", tag);
    for(uint8_t i=0; i<length; i++){

        if (str[i]<32){

            ets_printf("[%x] ", str[i]);
        }
        else{
            ets_printf("%c ", str[i]);

        }
    }
    ets_printf("\r\n");


};


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


    uint8_t ready_flags = ((uint8_t*) trans->rx_buffer)[GRID_PARAMETER_SPI_STATUS_FLAGS_index];

    //ets_printf("%d\r\n", ready_flags);


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

    //ets_debug_string("SPI", trans->rx_buffer);


    spi_slave_transaction_t *result;

    //spi_slave_get_trans_result(RCV_HOST, &result, 0);
    //grid_platform_printf("@ SPI COMPLETE: ");

}




uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length){

    uint8_t dir_index = direction-GRID_CONST_NORTH;


    spi_slave_transaction_t* t = &outbnound_transaction[dir_index];


    ((uint8_t*)t->tx_buffer)[length] = 0; // termination zero fter the message
    ((uint8_t*)t->tx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = (1<<dir_index);


    //ets_printf("SEND %d: %s\r\n", dir_index, buffer);

    spi_slave_queue_trans(RCV_HOST, t, portMAX_DELAY);
    queue_state++;

    spi_ready = 0;

    return 0; // done

}


void grid_esp32_port_task(void *arg)
{


    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;


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

    WORD_ALIGNED_ATTR char sendbuf[GRID_PARAMETER_SPI_TRANSACTION_length+1]={0};
    WORD_ALIGNED_ATTR char recvbuf[GRID_PARAMETER_SPI_TRANSACTION_length+1]={0};
    

    //Clear receive buffer, set send buffer to something sane
    memset(recvbuf, 0xA5, GRID_PARAMETER_SPI_TRANSACTION_length+1);        
    sprintf(sendbuf, "This is the receiver, sending data for transmission number %04d.", n);


    struct grid_port* port_list[4] = {&GRID_PORT_N, &GRID_PORT_E, &GRID_PORT_S, &GRID_PORT_W};

    for (uint8_t i = 0; i<4; i++){

        //Set up a transaction of GRID_PARAMETER_SPI_TRANSACTION_length bytes to send/receive

        memset(&outbnound_transaction[i], 0, sizeof(outbnound_transaction[i]));
        outbnound_transaction[i].length = GRID_PARAMETER_SPI_TRANSACTION_length*8;
        outbnound_transaction[i].tx_buffer=port_list[i]->tx_double_buffer;
        outbnound_transaction[i].rx_buffer=recvbuf;
    }


    empty_tx_buffer[0] = 'X';

    spi_empty_transaction.length=GRID_PARAMETER_SPI_TRANSACTION_length*8;
    spi_empty_transaction.tx_buffer=empty_tx_buffer;
    spi_empty_transaction.rx_buffer=recvbuf;





    //GRID_PORT_N.partner_status = 1; // force connected
    //GRID_PORT_E.partner_status = 1; // force connected
    //GRID_PORT_S.partner_status = 1; // force connected
    //GRID_PORT_W.partner_status = 1; // force connected

    static uint32_t loopcounter = 0;
    //ret=spi_slave_queue_trans(RCV_HOST, &t, 0);
    ret=spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, portMAX_DELAY);
    queue_state++;
    //spi_ready = 0;


    uint8_t pingcounter = 0;

    while (1) {

        if (xSemaphoreTake(signaling_sem, portMAX_DELAY) == pdTRUE){


            if (queue_state == 0){
                spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, portMAX_DELAY);
                queue_state++;
            }




            if (spi_ready == 1){



                n++;
                sprintf(sendbuf, "This is the receiver, sending data for transmission number %04d.", n);



            // grid_platform_printf("@ READY COMPLETE: ");
                spi_slave_transaction_t *trans = NULL;
                spi_slave_get_trans_result(RCV_HOST, &trans, portMAX_DELAY);

                //ets_printf("RX status,source: %d,%d : %s\r\n", ((uint8_t*) trans->rx_buffer)[GRID_PARAMETER_SPI_STATUS_FLAGS_index], ((uint8_t*) trans->rx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index], ((uint8_t*) trans->rx_buffer));
            //  grid_platform_printf("@%d: %s %s\r\n", trans->length, trans->tx_buffer, trans->rx_buffer);




                // figure out where the message came from (which port)
                uint8_t source_flags = ((uint8_t*) trans->rx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];

                struct grid_port* port = NULL;



                if ((source_flags&0b00000001)){
                    port = &GRID_PORT_N;
                }

                if ((source_flags&0b00000010)){
                    port = &GRID_PORT_E;
                }

                if ((source_flags&0b00000100)){
                    port = &GRID_PORT_S;
                }

                if ((source_flags&0b00001000)){
                    port = &GRID_PORT_W;
                }   

                if (port != NULL){
                    // we found the port in question

                    strcpy(port->rx_double_buffer, (char*) trans->rx_buffer);

                    port->rx_double_buffer_timeout = 0;
                    port->rx_double_buffer_read_start_index = 0;
                    port->rx_double_buffer_seek_start_index = 0;
                    


                    ets_debug_string("PRE", port->rx_double_buffer);


                    for(uint16_t i = 0; i<490; i++){ // 490 is the max processing length
                            
                        if (port->rx_double_buffer[port->rx_double_buffer_seek_start_index] == 10){ // \n
                                
                            port->rx_double_buffer_status = 1;
                            uint16_t length = port->rx_double_buffer_seek_start_index - port->rx_double_buffer_read_start_index + 1;
                            grid_port_receive_decode(port, length);

                            port->rx_double_buffer_status = 0;
                            port->rx_double_buffer_read_start_index = port->rx_double_buffer_seek_start_index;

                        }
                        else if (port->rx_double_buffer[port->rx_double_buffer_seek_start_index] == 0){
                            
                            break;
                        }


                        port->rx_double_buffer_seek_start_index++;
                            
                    }

                }


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


            grid_port_process_inbound(&GRID_PORT_U, 1); // Loopback , put rx_buffer content to each CONNECTED port's tx_buffer

            
            
            
            // ... GRID UART PORTS ...
            

            grid_port_process_inbound(&GRID_PORT_H, 0);


            grid_port_process_inbound(&GRID_PORT_N, 0);
            grid_port_process_inbound(&GRID_PORT_E, 0);
            grid_port_process_inbound(&GRID_PORT_S, 0);
            grid_port_process_inbound(&GRID_PORT_W, 0);


            // OUTBOUND
            // ... GRID UART PORTS ...
            

            grid_port_process_outbound_usb(&GRID_PORT_H); 


 
        
            grid_port_process_outbound_ui(&GRID_PORT_U);



            //printf("LOOP %d\r\n", queue_state);

            pingcounter++;
            if (pingcounter%10 == 0){
                //ets_printf("TRY PING\r\n");

                GRID_PORT_N.ping_flag = 1;
                GRID_PORT_E.ping_flag = 1;
                GRID_PORT_S.ping_flag = 1;
                GRID_PORT_W.ping_flag = 1;

                grid_port_ping_try_everywhere();
                
            }


            //grid_port_process_outbound_usart(&GRID_PORT_N);

            for (uint8_t i=0; i<4; i++){
                struct grid_port* port = port_list[i];
                grid_port_process_outbound_usart(port);
            }

        

            for (uint8_t i=0; i<4; i++){

                struct grid_port* port = port_list[i];

                uint32_t limit = 1000;

                if (port->rx_double_buffer_timeout<limit){
                
                    port->rx_double_buffer_timeout+=10;

   
                    if (port->rx_double_buffer_timeout>=limit){

                        if (port->partner_status == 1){

                            grid_port_receiver_softreset(port);	
                            //grid_port_receiver_softreset(por);	
                                    

                            grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
                            grid_led_set_alert_frequency(&grid_led_state, -2);	
                            grid_led_set_alert_phase(&grid_led_state, 100);	

                        }

                            ets_printf("DISCONNECT\r\n");
                    }
                }


            }



            xSemaphoreGive(signaling_sem);

        }



        vTaskDelay(pdMS_TO_TICKS(4));



    }


    ESP_LOGI(TAG, "Deinit PORT");
    vTaskSuspend(NULL);
}

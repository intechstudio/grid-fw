/**
 *
 * On-Board LED Blinky
 */


#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"

#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/spi.h"

#include "uart_tx.pio.h"
#include "uart_rx.pio.h"

#include "hardware/uart.h"

#include <string.h>

#define TEST_SIZE 512

uint dma_tx;
uint dma_rx; 



static uint8_t txbuf[TEST_SIZE];
static uint8_t rxbuf[TEST_SIZE];

const uint CS_PIN = 17; // was 13

uint8_t ready_flags = 255;


// PIO SETUP CONSTANTS    
const uint SERIAL_BAUD = 2000000UL;

const PIO GRID_TX_PIO = pio0;
const PIO GRID_RX_PIO = pio1;

const uint GRID_NORTH_SM = 0;
const uint GRID_EAST_SM  = 1;
const uint GRID_SOUTH_SM = 2;
const uint GRID_WEST_SM  = 3;


// GRID UART PIN CONNECTIONS
const uint GRID_NORTH_TX_PIN = 23;
const uint GRID_NORTH_RX_PIN = 6;

const uint GRID_EAST_TX_PIN = 26;
const uint GRID_EAST_RX_PIN = 24;

const uint GRID_SOUTH_TX_PIN = 2;
const uint GRID_SOUTH_RX_PIN = 27;

const uint GRID_WEST_TX_PIN = 5;
const uint GRID_WEST_RX_PIN = 3;

// GRID SYNC PIN CONNECTIONS
const uint GRID_SYNC_1_PIN = 25;
const uint GRID_SYNC_2_PIN = 4;

volatile uint8_t spi_dma_done = true;

void dma_handler() {

    spi_dma_done = true;
    gpio_put(CS_PIN, 1);
    dma_hw->ints0 = 1u << dma_rx;

    //printf("FINISH %d\n", txbuf[499]);

}

uint8_t NORTH_tx_buffer[512] = {0};
uint8_t NORTH_tx_is_busy = 0;
uint16_t NORTH_tx_index = 0;


uint8_t WEST_tx_buffer[] = {0x1, 0x0E, 0x07, 0x11, 0x34, 0x31, 0x66, 0x66, 0x66, 0x66, 0x04, 0x31, 0x38, 0x0A, 0x00};
uint8_t WEST_tx_is_busy = 0;
uint16_t WEST_tx_index = 0;

int has_even_parity(uint8_t x){

    uint8_t count = 0, i, b = 1;

    for(i = 0; i < 8; i++){
        if( x & (b << i) ){count++;}
    }

    if( (count % 2) ){return 0;}

    return 1;
}



void init_pio_quad_uart(void){

    // INITIALIZE PIO UART TX

    uint offset_tx = pio_add_program(GRID_TX_PIO, &uart_tx_program);
    uart_tx_program_init(GRID_TX_PIO, GRID_NORTH_SM, offset_tx, GRID_NORTH_TX_PIN, SERIAL_BAUD);
    uart_tx_program_init(GRID_TX_PIO, GRID_EAST_SM, offset_tx, GRID_EAST_TX_PIN, SERIAL_BAUD);
    uart_tx_program_init(GRID_TX_PIO, GRID_SOUTH_SM, offset_tx, GRID_SOUTH_TX_PIN, SERIAL_BAUD);
    uart_tx_program_init(GRID_TX_PIO, GRID_WEST_SM, offset_tx, GRID_WEST_TX_PIN, SERIAL_BAUD);
    

    // INITIALIZE PIO UART RX

    uint offset_rx = pio_add_program(GRID_RX_PIO, &uart_rx_program);
    uart_rx_program_init(GRID_RX_PIO, GRID_NORTH_SM, offset_rx, GRID_NORTH_RX_PIN, SERIAL_BAUD);
    uart_rx_program_init(GRID_RX_PIO, GRID_EAST_SM, offset_rx, GRID_EAST_RX_PIN, SERIAL_BAUD);
    uart_rx_program_init(GRID_RX_PIO, GRID_SOUTH_SM, offset_rx, GRID_SOUTH_RX_PIN, SERIAL_BAUD);
    uart_rx_program_init(GRID_RX_PIO, GRID_WEST_SM, offset_rx, GRID_WEST_RX_PIN, SERIAL_BAUD);


}


void spi_start_transfer(uint tx_channel, uint rx_channel, uint8_t* tx_buffer, uint8_t* rx_buffer, irq_handler_t callback){

    dma_channel_config c = dma_channel_get_default_config(tx_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
    dma_channel_configure(tx_channel, &c,
                        &spi_get_hw(spi_default)->dr, // write address
                        tx_buffer, // read address
                        TEST_SIZE, // element count (each element is of size transfer_data_size)
                        false); // don't start yet


    c = dma_channel_get_default_config(rx_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure(rx_channel, &c,
                        rx_buffer, // write address
                        &spi_get_hw(spi_default)->dr, // read address
                        TEST_SIZE, // element count (each element is of size transfer_data_size)
                        false); // don't start yet


    if (callback != NULL){
        dma_channel_set_irq0_enabled(rx_channel, true);
        irq_set_exclusive_handler(DMA_IRQ_0, callback);
        irq_set_enabled(DMA_IRQ_0, true);
    }



    dma_start_channel_mask((1u << tx_channel) | (1u << rx_channel));


}


int main() 
{

    stdio_init_all();
    // SPI INIT

    // Setup COMMON stuff
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_put(CS_PIN, 1);


    spi_init(spi_default, 31250 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);

    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    dma_tx = dma_claim_unused_channel(true);
    dma_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    //hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

    for (uint i = 0; i < TEST_SIZE; ++i) {
        txbuf[i] = 'a'+i%20;
    }
    

    spi_start_transfer(dma_tx, dma_rx, txbuf, rxbuf, NULL);

    printf("Wait for RX complete...\n");
    dma_channel_wait_for_finish_blocking(dma_rx);
    if (dma_channel_is_busy(dma_tx)) {
        panic("RX completed before TX");
    }


    // Setup COMMON stuff
    const uint LED_PIN = 15; // was 25 =  PICO_DEFAULT_LED_PIN
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);




    // INITIALIZE PIO UART
    init_pio_quad_uart();

    uint8_t loopcouter = 0;    
    
    uint32_t loopcouter2 = 0;

    
    uint8_t spi_counter = 0;

    printf("Init Complete...\n");

    while (1) 
    {
        loopcouter++;
        loopcouter2++;

        loopcouter%=5;

        if (loopcouter2 > 5000){
            gpio_put(LED_PIN, 1);

        }
        if (loopcouter2 > 10000){
            loopcouter2 = 0;
            gpio_put(LED_PIN, 0);



            //uart_tx_program_putc(GRID_TX_PIO, GRID_EAST_SM, 0b11011011);

            uint32_t toSend = 0x01;

            if (has_even_parity(toSend)){
               
                toSend |= 0b100000000;
            }
            else{
            }

            pio_sm_put_blocking(GRID_TX_PIO, GRID_EAST_SM, toSend);

            uart_tx_program_putc(GRID_TX_PIO, GRID_SOUTH_SM, '2'+loopcouter);



            // DMA TEST

            // We set the outbound DMA to transfer from a memory buffer to the SPI transmit FIFO paced by the SPI TX FIFO DREQ
            // The default is for the read address to increment every element (in this case 1 byte = DMA_SIZE_8)
            // and for the write address to remain unchanged.


            if (dma_channel_is_busy(dma_rx)){

            }
            else{




                if (spi_dma_done){

                    for (uint8_t i = 0; i<10; i++){

                        //printf(" 0x%02x ", rxbuf[i]);
                    }

                    //printf("RX[499] = %d: %s ", rxbuf[499], rxbuf);



                    spi_dma_done = false;


                    gpio_put(CS_PIN, 0);


                    sprintf(txbuf, "Test Count: %3d |||", spi_counter);
                    spi_counter++;


                    


                    uint8_t busy_flags = rxbuf[499];





                    if ((busy_flags&0b00000001)){

                        NORTH_tx_is_busy = 1;
                        NORTH_tx_index = 0;
                        strcpy(NORTH_tx_buffer, rxbuf);
                        
                        ready_flags &= ~(1<<0); // clear north ready

                    }

                    if ((busy_flags&0b00000010)){
                        ready_flags |= (1<<1);
                    }

                    if ((busy_flags&0b00000100)){
                        ready_flags |= (1<<2);
                    }

                    if ((busy_flags&0b00001000)){
                        ready_flags |= (1<<3);
                    }   


                    txbuf[499] = ready_flags;


                    //printf("START %d\n", txbuf[499]);


                    spi_start_transfer(dma_tx, dma_rx, txbuf, rxbuf, dma_handler);

                    if (WEST_tx_is_busy == 0){
                        WEST_tx_is_busy = 1;
                        WEST_tx_index = 0;
                    }


                }




            }


        }


        if (NORTH_tx_is_busy){

            char c = NORTH_tx_buffer[NORTH_tx_index];
            NORTH_tx_index++;

            uart_tx_program_putc(GRID_TX_PIO, GRID_NORTH_SM, c);

            if (c == '\n'){

                NORTH_tx_is_busy = 0;

                ready_flags |= (1<<0);
            }

        }


        
        if (uart_rx_program_is_available(GRID_RX_PIO, GRID_NORTH_SM)){
            char c = uart_rx_program_getc(GRID_RX_PIO, GRID_NORTH_SM);
            printf("N: %c\r\n", c);
        }

        if (uart_rx_program_is_available(GRID_RX_PIO, GRID_EAST_SM)){
            char c = uart_rx_program_getc(GRID_RX_PIO, GRID_EAST_SM);
            printf("E: %c\r\n", c);
        }

        if (uart_rx_program_is_available(GRID_RX_PIO, GRID_SOUTH_SM)){
            char c = uart_rx_program_getc(GRID_RX_PIO, GRID_SOUTH_SM);
            printf("S: %c\r\n", c);
        }

        if (uart_rx_program_is_available(GRID_RX_PIO, GRID_WEST_SM)){
            char c = uart_rx_program_getc(GRID_RX_PIO, GRID_WEST_SM);
            printf("W: %c\r\n", c);
        }





        
    }
}

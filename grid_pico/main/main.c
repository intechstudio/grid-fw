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


#define TEST_SIZE 512

uint dma_tx;
uint dma_rx; 


const uint CS_PIN = 17; // was 13

volatile uint8_t spi_dma_done = true;

void dma_handler() {

    spi_dma_done = true;
    gpio_put(CS_PIN, 1);
    dma_hw->ints0 = 1u << dma_rx;

}


int main() 
{

    stdio_init_all();
    // SPI INIT

    // Setup COMMON stuff
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_put(CS_PIN, 1);


    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);

    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    dma_tx = dma_claim_unused_channel(true);
    dma_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

    static uint8_t txbuf[TEST_SIZE];
    static uint8_t rxbuf[TEST_SIZE];
    for (uint i = 0; i < TEST_SIZE; ++i) {
        txbuf[i] = 'a'+i%20;
    }
    
    // We set the outbound DMA to transfer from a memory buffer to the SPI transmit FIFO paced by the SPI TX FIFO DREQ
    // The default is for the read address to increment every element (in this case 1 byte = DMA_SIZE_8)
    // and for the write address to remain unchanged.

    printf("Configure TX DMA\n");
    dma_channel_config c = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
    dma_channel_configure(dma_tx, &c,
                          &spi_get_hw(spi_default)->dr, // write address
                          txbuf, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet

    printf("Configure RX DMA\n");

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We configure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    c = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure(dma_rx, &c,
                          rxbuf, // write address
                          &spi_get_hw(spi_default)->dr, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet


    printf("Starting DMAs...\n");

    // start them exactly simultaneously to avoid races (in extreme cases the FIFO could overflow)
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
    printf("Wait for RX complete...\n");
    dma_channel_wait_for_finish_blocking(dma_rx);
    if (dma_channel_is_busy(dma_tx)) {
        panic("RX completed before TX");
    }


    // Setup COMMON stuff
    const uint LED_PIN = 15; // was 25 =  PICO_DEFAULT_LED_PIN
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);



    
    const uint SERIAL_BAUD = 2000000UL;


    // Setup PIO UART TX
    const uint PIN_TX_N = 10;
    const uint PIN_TX_E = 11;
    
    PIO pio_N = pio0;
    uint sm_N = 0;
    uint offset_N = pio_add_program(pio_N, &uart_tx_program);
    uart_tx_program_init(pio_N, sm_N, offset_N, PIN_TX_N, SERIAL_BAUD);

    PIO pio_E = pio0;
    uint sm_E = 1;
    uint offset_E = pio_add_program(pio_E, &uart_tx_program);
    uart_tx_program_init(pio_E, sm_E, offset_E, PIN_TX_E, SERIAL_BAUD);
    
    // Setup PIO UART RX
    const uint PIN_RX_N = 3;

    PIO pio = pio1;
    uint sm = 0;
    uint offset = pio_add_program(pio, &uart_rx_program);
    uart_rx_program_init(pio, sm, offset, PIN_RX_N, SERIAL_BAUD);


    uint8_t loopcouter = 0;    
    
    uint32_t loopcouter2 = 0;

    
    uint8_t spi_counter = 0;

    while (1) 
    {
        loopcouter++;
        loopcouter2++;

        loopcouter%=10;

        if (loopcouter2 > 50000){
            gpio_put(LED_PIN, 1);
        }
        if (loopcouter2 > 100000){
            loopcouter2 = 0;
            gpio_put(LED_PIN, 0);





            // DMA TEST

            // We set the outbound DMA to transfer from a memory buffer to the SPI transmit FIFO paced by the SPI TX FIFO DREQ
            // The default is for the read address to increment every element (in this case 1 byte = DMA_SIZE_8)
            // and for the write address to remain unchanged.


            if (dma_channel_is_busy(dma_rx)){

            }
            else{




                if (spi_dma_done){
                    spi_dma_done = false;


                    gpio_put(CS_PIN, 0);


                    sprintf(txbuf, "Test Count: %3d |||", spi_counter);
                    spi_counter++;


                    printf("Starting DMAs...\n");



                    printf("Configure TX DMA\n");
                    dma_channel_config c = dma_channel_get_default_config(dma_tx);
                    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
                    channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
                    dma_channel_configure(dma_tx, &c,
                                        &spi_get_hw(spi_default)->dr, // write address
                                        txbuf, // read address
                                        TEST_SIZE, // element count (each element is of size transfer_data_size)
                                        false); // don't start yet

                    printf("Configure RX DMA\n");

                    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
                    // We configure the read address to remain unchanged for each element, but the write
                    // address to increment (so data is written throughout the buffer)
                    c = dma_channel_get_default_config(dma_rx);
                    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
                    channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
                    channel_config_set_read_increment(&c, false);
                    channel_config_set_write_increment(&c, true);
                    dma_channel_configure(dma_rx, &c,
                                        rxbuf, // write address
                                        &spi_get_hw(spi_default)->dr, // read address
                                        TEST_SIZE, // element count (each element is of size transfer_data_size)
                                        false); // don't start yet


                    dma_channel_set_irq0_enabled(dma_rx, true);

                    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
                    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
                    irq_set_enabled(DMA_IRQ_0, true);

                    // start them exactly simultaneously to avoid races (in extreme cases the FIFO could overflow)
                    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
                    printf("Wait for RX complete...\n");
                    //dma_channel_wait_for_finish_blocking(dma_rx);
                    // if (dma_channel_is_busy(dma_tx)) {
                    //     panic("RX completed before TX");
                    // }


                }




            }


        }


        uart_tx_program_putc(pio_N, sm_N, '0'+loopcouter);


        if (uart_rx_program_is_available(pio, sm)){
            char c = uart_rx_program_getc(pio, sm);
            uint8_t buffer[10] = {0};
            sprintf(buffer, "%c", c);

            uart_tx_program_puts(pio_E, sm_E, buffer);
        }





        
    }
}

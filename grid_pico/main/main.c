/**
 *
 * On-Board LED Blinky
 */


#include "pico/stdlib.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"
#include "uart_rx.pio.h"

#include "hardware/uart.h"

int main() 
{

    // Setup COMMON stuff
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);


    gpio_init(15);
    gpio_set_dir(15, GPIO_OUT);
    gpio_put(15, 1);
    
    const uint SERIAL_BAUD = 2000000UL;


    // Setup PIO UART TX
    const uint PIN_TX_N = 0;
    const uint PIN_TX_E = 1;
    
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

    


    while (1) 
    {
        loopcouter++;
        loopcouter2++;

        loopcouter%=10;

        if (loopcouter2 > 10000){
            gpio_put(LED_PIN, 1);
        }
        if (loopcouter2 > 20000){
            loopcouter2 = 0;
            gpio_put(LED_PIN, 0);
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

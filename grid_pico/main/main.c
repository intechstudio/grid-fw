/**
 *
 * On-Board LED Blinky
 */


#include "pico/stdlib.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"

int main() 
{
    // We're going to use PIO to print "Hello, world!" on the same GPIO which we
    // normally attach UART0 to.
    const uint PIN_TX_N = 0;
    const uint PIN_TX_E = 1;
    
    const uint SERIAL_BAUD = 10000000UL;

    PIO pio_N = pio0;
    uint sm_N = 0;
    uint offset_N = pio_add_program(pio_N, &uart_tx_program);
    uart_tx_program_init(pio_N, sm_N, offset_N, PIN_TX_N, SERIAL_BAUD);

    PIO pio_E = pio0;
    uint sm_E = 1;
    uint offset_E = pio_add_program(pio_E, &uart_tx_program);
    uart_tx_program_init(pio_E, sm_E, offset_E, PIN_TX_E, SERIAL_BAUD);


    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while (1) 
    {

        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(1000);


        uart_tx_program_puts(pio_N, sm_N, "U");

        uart_tx_program_puts(pio_E, sm_E, "F");

        
    }
}

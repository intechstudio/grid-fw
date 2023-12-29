/**
 *
 * On-Board LED Blinky
 */

#include <stdio.h>
#include <stdlib.h>

#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"

#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#include "hardware/uart.h"

#include "pico/multicore.h"

#include "../../grid_common/grid_protocol.h"

#include <string.h>

#include "pico/time.h"

uint8_t grid_msg_string_calculate_checksum_of_packet_string(char *str,
                                                            uint32_t length) {

  uint8_t checksum = 0;
  for (uint32_t i = 0; i < length - 3; i++) {
    checksum ^= str[i];
  }

  return checksum;
}
uint8_t grid_msg_string_read_hex_char_value(uint8_t ascii,
                                            uint8_t *error_flag) {

  uint8_t result = 0;

  if (ascii > 47 && ascii < 58) {
    result = ascii - 48;
  } else if (ascii > 96 && ascii < 103) {
    result = ascii - 97 + 10;
  } else {
    // wrong input
    if (error_flag != 0) {
      *error_flag = ascii;
    }
  }

  return result;
}
uint32_t grid_msg_string_read_hex_string_value(char *start_location,
                                               uint8_t length,
                                               uint8_t *error_flag) {

  uint32_t result = 0;

  for (uint8_t i = 0; i < length; i++) {

    result += grid_msg_string_read_hex_char_value(start_location[i], error_flag)
              << (length - i - 1) * 4;
  }

  return result;
}
uint8_t grid_msg_string_checksum_read(char *str, uint32_t length) {
  uint8_t error_flag;
  return grid_msg_string_read_hex_string_value(&str[length - 3], 2,
                                               &error_flag);
}

#define BUCKET_BUFFER_LENGTH 500
#define BUCKET_ARRAY_LENGTH 50

uint dma_tx;
uint dma_rx;

static uint8_t txbuf[GRID_PARAMETER_SPI_TRANSACTION_length];
static uint8_t rxbuf[GRID_PARAMETER_SPI_TRANSACTION_length];

const uint CS_PIN = 17; // was 13

uint8_t ready_flags = 255;

// PIO SETUP CONSTANTS
const uint SERIAL_BAUD = 2000000UL;

const PIO GRID_TX_PIO = pio0;
const PIO GRID_RX_PIO = pio1;

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
const uint LED_PIN = 15; // was 25 =  PICO_DEFAULT_LED_PIN
const uint SYNC1_PIN = 25;
const uint SYNC2_PIN = 4;

volatile uint8_t spi_dma_done = false;

void dma_handler() {

  spi_dma_done = true;
  gpio_put(CS_PIN, 1);
  dma_hw->ints0 = 1u << dma_rx;

  // printf("FINISH %d\n", txbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index]);
}

enum grid_bucket_status_t {

  GRID_BUCKET_STATUS_EMPTY,
  GRID_BUCKET_STATUS_RECEIVING,
  GRID_BUCKET_STATUS_FULL,
  GRID_BUCKET_STATUS_TRANSMITTING,
  GRID_BUCKET_STATUS_DONE,

};

struct grid_bucket {

  enum grid_bucket_status_t status;
  uint8_t index;
  uint8_t source_port_index;
  uint8_t buffer[512];
  uint16_t buffer_index;
};

struct grid_port {

  uint8_t port_index;
  uint8_t tx_buffer[512];
  uint8_t tx_is_busy;
  uint16_t tx_index;

  struct grid_bucket *active_bucket;
};

uint8_t bucket_array_length = BUCKET_ARRAY_LENGTH;
struct grid_bucket bucket_array[BUCKET_ARRAY_LENGTH];

struct grid_port port_array[4];

struct grid_port *NORTH = &port_array[0];
struct grid_port *EAST = &port_array[1];
struct grid_port *SOUTH = &port_array[2];
struct grid_port *WEST = &port_array[3];

void grid_port_init(struct grid_port *port, uint8_t index) {

  port->port_index = index;

  for (uint16_t i = 0; i < 512; i++) {
    port->tx_buffer[i] = 0;
  }

  port->tx_index = 0;
  port->tx_is_busy = false;

  port->active_bucket = NULL;
}

void grid_bucket_init(struct grid_bucket *bucket, uint8_t index) {

  bucket->status = GRID_BUCKET_STATUS_EMPTY;
  bucket->index = index;

  bucket->source_port_index = 255;

  // for (uint16_t i=0; i<512; i++){
  //     bucket->buffer[i] = 0;
  // }

  bucket->buffer_index = 0;
}

// find next bucket, so UART can safely receive data into it
struct grid_bucket *
grid_bucket_find_next_match(struct grid_bucket *previous_bucket,
                            enum grid_bucket_status_t expected_status) {

  // if no previous bucket was given then start from the end of the array (+1
  // will make this the beginning of the array later)
  if (previous_bucket == NULL) {
    previous_bucket = &bucket_array[bucket_array_length - 1];
  }

  for (uint8_t i = 0; i < bucket_array_length; i++) {

    uint8_t next_index = (previous_bucket->index + i + 1) % bucket_array_length;

    if (bucket_array[next_index].status == expected_status) {

      // printf("Bucket 4 u : %d\r\n", next_index);
      return &bucket_array[next_index];
    }
  }

  // printf("No bucket 4 u :(\r\n");
  return NULL;
}

struct grid_bucket *spi_active_bucket = NULL;
struct grid_bucket *spi_previous_bucket = NULL;

void grid_bucket_put_character(struct grid_bucket *bucket, char next_char) {

  if (bucket == NULL) {

    printf("PUTC\n");
    return;
  }

  if (bucket->buffer_index < BUCKET_BUFFER_LENGTH) {

    bucket->buffer[bucket->buffer_index] = next_char;
    bucket->buffer_index++;

  } else {
    printf("PUTC: no more space");
  }
}

void spi_start_transfer(uint tx_channel, uint rx_channel, uint8_t *tx_buffer,
                        uint8_t *rx_buffer, irq_handler_t callback) {

  spi_dma_done = false;
  gpio_put(CS_PIN, 0);

  dma_channel_config c = dma_channel_get_default_config(tx_channel);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
  dma_channel_configure(
      tx_channel, &c,
      &spi_get_hw(spi_default)->dr,          // write address
      tx_buffer,                             // read address
      GRID_PARAMETER_SPI_TRANSACTION_length, // element count (each element is
                                             // of size transfer_data_size)
      false);                                // don't start yet

  c = dma_channel_get_default_config(rx_channel);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  dma_channel_configure(
      rx_channel, &c,
      rx_buffer,                             // write address
      &spi_get_hw(spi_default)->dr,          // read address
      GRID_PARAMETER_SPI_TRANSACTION_length, // element count (each element is
                                             // of size transfer_data_size)
      false);                                // don't start yet

  if (callback != NULL) {
    dma_channel_set_irq0_enabled(rx_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, callback);
    irq_set_enabled(DMA_IRQ_0, true);
  }

  dma_start_channel_mask((1u << tx_channel) | (1u << rx_channel));
}

void spi_interface_init() {
  // SPI INIT

  // Setup COMMON stuff
  gpio_init(CS_PIN);
  gpio_set_dir(CS_PIN, GPIO_OUT);
  gpio_put(CS_PIN, 1);

  uint baudrate = spi_init(spi_default, 31250 * 1000);
  printf("BAUD: %d", baudrate);

  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

  // Force loopback for testing (I don't have an SPI device handy)
  // hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

  dma_tx = dma_claim_unused_channel(true);
  dma_rx = dma_claim_unused_channel(true);
}

void grid_port_attach_bucket(struct grid_port *port) {

  port->active_bucket = grid_bucket_find_next_match(port->active_bucket,
                                                    GRID_BUCKET_STATUS_EMPTY);

  if (port->active_bucket != NULL) {

    port->active_bucket->status = GRID_BUCKET_STATUS_RECEIVING;
    port->active_bucket->source_port_index = port->port_index;

  } else {

    printf("NULL BUCKET\r\n");
  }
}

void uart_try_send_all(void) {

  // iterate through all the ports
  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port *port = &port_array[i];

    // if transmission is in progress then send the next character
    if (port->tx_is_busy) {

      char c = port->tx_buffer[port->tx_index];
      port->tx_index++;

      uart_tx_program_putc(GRID_TX_PIO, port->port_index, c);

      if (c == '\n') {

        port->tx_is_busy = 0;
        ready_flags |= (1 << port->port_index);
      }
    }
  }
}

void fifo_try_receive(void) {

  uint32_t data = 0;
  uint32_t status = 0;
  // continue;

  while (multicore_fifo_rvalid()) {

    data = multicore_fifo_pop_blocking();

    // printf("POP");

    for (uint8_t i = 0; i < 4; i++) {

      uint8_t c = (data >> (8 * i)) & 0x000000FF;

      if (c == 0) {
        continue;
      }

      struct grid_port *port = &port_array[i];

      if (port->active_bucket == NULL) {
        grid_port_attach_bucket(port);
      }

      if (c == 0x01 && port->active_bucket->buffer_index >
                           0) { // Start of Header character received in the
                                // middle of a trasmisdsion

        printf("E\n");

        // clear the current active bucet, and attach to a new bucket to start
        // receiving packet
        grid_bucket_init(port->active_bucket, port->active_bucket->index);
        port->active_bucket == NULL;
        grid_port_attach_bucket(port);
      }

      grid_bucket_put_character(port->active_bucket, c);

      // printf("%c", c);

      if (c == '\n') {

        // end of message, put termination zero character
        grid_bucket_put_character(port->active_bucket, '\0');

        // printf("BUCKET READY %s\r\n", port->active_bucket->buffer);
        if (port->active_bucket->buffer[1] == GRID_CONST_BRC) {
          // printf("BR%d %c%c\r\n", port->active_bucket->index,
          // port->active_bucket->buffer[6], port->active_bucket->buffer[7]);
        }
        port->active_bucket->status = GRID_BUCKET_STATUS_FULL;
        port->active_bucket->buffer_index = 0;
        // clear bucket

        grid_port_attach_bucket(port);
      }
    }
  }
}

void core_1_main_entry() {

  printf("Core 1 init\r\n");

  while (1) {

    uint32_t packed_chars = 0;

    // iterate through all the ports and pack available characters
    for (uint8_t i = 0; i < 4; i++) {
      if (uart_rx_program_is_available(GRID_RX_PIO, i)) {

        char c = uart_rx_program_getc(GRID_RX_PIO, i);
        packed_chars |= (c << (8 * i));
      }
    }

    if (packed_chars != 0) {

      uint8_t ok = multicore_fifo_push_timeout_us(packed_chars, 0);

      if (!ok) {
        printf("F\n");
      }
    }
  }
}

void core0_interrupt_handler(void) {

  fifo_try_receive();
  multicore_fifo_clear_irq();
}

volatile uint8_t sync1_drive = 0;
volatile uint8_t sync1_interrupt = 0;
volatile uint8_t sync2_interrupt = 0;

void gpio_sync_pin_callback(uint gpio, uint32_t events) {

  if (gpio == SYNC1_PIN) {

    sync1_interrupt = 1;
  } else if (gpio == SYNC2_PIN) {

    sync2_interrupt = 1;
  }
}

void spi_txbuffer_set_sync_state(uint8_t *buff) {

  buff[GRID_PARAMETER_SPI_SYNC1_STATE_index] = 0;
  buff[GRID_PARAMETER_SPI_SYNC2_STATE_index] = 0;

  if (sync1_interrupt) {
    sync1_interrupt = 0;
    buff[GRID_PARAMETER_SPI_SYNC1_STATE_index] = 1;
  }
  if (sync2_interrupt) {
    sync2_interrupt = 0;
    buff[GRID_PARAMETER_SPI_SYNC2_STATE_index] = 1;
  }
}

int main() {

  stdio_init_all();
  printf("RP2040 START\r\n");

  uint offset_tx = pio_add_program(GRID_TX_PIO, &uart_tx_program);
  uint offset_rx = pio_add_program(GRID_RX_PIO, &uart_rx_program);

  // setup gpio sync interrupts
  gpio_set_irq_enabled_with_callback(SYNC1_PIN, GPIO_IRQ_EDGE_RISE, true,
                                     &gpio_sync_pin_callback);
  gpio_set_irq_enabled(SYNC2_PIN, GPIO_IRQ_EDGE_RISE, true);

  grid_port_init(NORTH, 0);
  grid_port_init(EAST, 1);
  grid_port_init(SOUTH, 2);
  grid_port_init(WEST, 3);

  for (uint8_t i = 0; i < bucket_array_length; i++) {
    grid_bucket_init(&bucket_array[i], i);
  }

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port *port = &port_array[i];

    grid_port_attach_bucket(port);
  }

  uart_tx_program_init(GRID_TX_PIO, 0, offset_tx, GRID_NORTH_TX_PIN,
                       SERIAL_BAUD);
  uart_tx_program_init(GRID_TX_PIO, 1, offset_tx, GRID_EAST_TX_PIN,
                       SERIAL_BAUD);
  uart_tx_program_init(GRID_TX_PIO, 2, offset_tx, GRID_SOUTH_TX_PIN,
                       SERIAL_BAUD);
  uart_tx_program_init(GRID_TX_PIO, 3, offset_tx, GRID_WEST_TX_PIN,
                       SERIAL_BAUD);

  uart_rx_program_init(GRID_RX_PIO, 0, offset_rx, GRID_NORTH_RX_PIN,
                       SERIAL_BAUD);
  uart_rx_program_init(GRID_RX_PIO, 1, offset_rx, GRID_EAST_RX_PIN,
                       SERIAL_BAUD);
  uart_rx_program_init(GRID_RX_PIO, 2, offset_rx, GRID_SOUTH_RX_PIN,
                       SERIAL_BAUD);
  uart_rx_program_init(GRID_RX_PIO, 3, offset_rx, GRID_WEST_RX_PIN,
                       SERIAL_BAUD);

  gpio_init(SYNC1_PIN);
  gpio_init(SYNC2_PIN);

  // Setup COMMON stuff
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  spi_interface_init();

  for (uint i = 0; i < GRID_PARAMETER_SPI_TRANSACTION_length; i++) {
    txbuf[i] = 0;
  }

  uint32_t loopcouter = 0;
  uint8_t spi_counter = 0;

  struct grid_task {

    uint32_t start_timestamp;
    uint32_t interval;
  };

  void grid_task_init(struct grid_task * task, uint32_t interval) {

    task->interval = interval;
    task->start_timestamp = time_us_32();
  }

  uint8_t grid_task_should_run_now(struct grid_task * task) {

    if (time_us_32() - task->start_timestamp > task->interval) {

      task->start_timestamp = time_us_32();

      // should run now!
      return 1;

    } else {

      // should not run yet
      return 0;
    }
  }

  struct grid_task spi_receive_transfer_task;
  grid_task_init(&spi_receive_transfer_task, 100); // 1 ms interval

  struct grid_task spi_start_transfer_task;
  grid_task_init(&spi_start_transfer_task,
                 500); // 5 ms interval -> 1 ms -> 500us

  multicore_reset_core1();
  multicore_launch_core1(core_1_main_entry);

  multicore_fifo_clear_irq();
  irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_interrupt_handler);
  irq_set_enabled(SIO_IRQ_PROC0, true);

  while (1) {

    // TRY TO RECEIVE FROM SPI
    if (grid_task_should_run_now(&spi_receive_transfer_task)) {

      if (spi_dma_done) {

        spi_dma_done = false;
        uint8_t destination_flags =
            rxbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];

        uint8_t sync1_state = rxbuf[GRID_PARAMETER_SPI_SYNC1_STATE_index];

        if (sync1_state) {

          gpio_pull_down(SYNC1_PIN);
          sync1_drive = 1;
          gpio_set_dir(SYNC1_PIN, GPIO_OUT);
          gpio_put(SYNC1_PIN, 1);
          // set_drive_mode(SYNC1_PIN, GPIO_DRIVE_MODE_OPEN_DRAIN);

        } else if (sync1_drive) {
          gpio_set_dir(SYNC1_PIN, GPIO_IN);
          sync1_drive = 0;
        }

        // printf("%d\r\n", destination_flags);

        // iterate through all the ports
        for (uint8_t i = 0; i < 4; i++) {

          struct grid_port *port = &port_array[i];

          // copy message to the addressed port and set it to busy!
          if ((destination_flags & (1 << port->port_index))) {

            port->tx_is_busy = 1;
            port->tx_index = 0;
            strcpy(port->tx_buffer, rxbuf);
            // printf("SPI receive: %s\r\n", rxbuf);

            // printf("%02x %02x %02x %02x ...\r\n", rxbuf[0], rxbuf[1],
            // rxbuf[2], rxbuf[3]);

            ready_flags &= ~(1 << port->port_index); // clear ready
          }
        }

        if (spi_active_bucket != NULL) {
          // clear the bucket after use
          grid_bucket_init(spi_active_bucket, spi_active_bucket->index);
          spi_previous_bucket = spi_active_bucket;
          spi_active_bucket = NULL;
        }
      }
    }

    // TRY TO SEND THROUGH SPI
    if (grid_task_should_run_now(&spi_start_transfer_task)) {

      if (!dma_channel_is_busy(dma_rx)) {

        // printf("START %d\n", txbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index]);

        // try to send bucket content through SPI

        if (spi_active_bucket == NULL) {

          spi_active_bucket = grid_bucket_find_next_match(
              spi_previous_bucket, GRID_BUCKET_STATUS_FULL);

          // found full bucket, send it through SPI
          if (spi_active_bucket != NULL) {

            // printf("SPI send: %s\r\n", spi_active_bucket->buffer);

            spi_active_bucket->buffer[GRID_PARAMETER_SPI_STATUS_FLAGS_index] =
                ready_flags;
            spi_active_bucket->buffer[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] =
                (1 << (spi_active_bucket->source_port_index));

            // printf("BUCKET READY %s\r\n", port->active_bucket->buffer);

            // validate packet
            uint8_t error;

            uint16_t length = strlen(spi_active_bucket->buffer);
            uint16_t received_length = grid_msg_string_read_hex_string_value(
                &spi_active_bucket->buffer[GRID_BRC_LEN_offset], 4, &error);

            uint8_t calculated_checksum =
                grid_msg_string_calculate_checksum_of_packet_string(
                    spi_active_bucket->buffer, length);
            uint8_t received_checksum = grid_msg_string_checksum_read(
                spi_active_bucket->buffer, length);

            uint8_t error_count = 0;

            if (spi_active_bucket->buffer[1] == GRID_CONST_BRC) {

              if (length - 3 != received_length) {

                // printf("L%d %d ", length-3, received_length);

                error_count++;
              }
            }

            if (calculated_checksum != received_checksum) {

              // printf("C %d %d ", calculated_checksum, received_checksum);
              error_count++;
            }
            // printf("BR%d %c%c\r\n", spi_active_bucket->index,
            // spi_active_bucket->buffer[6], spi_active_bucket->buffer[7]);

            if (error_count > 0) {

              // printf("SKIP\r\n");
              printf("S\n");

              // send empty packet with status flags

              txbuf[0] = 0;
              sprintf(txbuf, "DUMMY ERROR");
              txbuf[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = ready_flags;
              txbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] =
                  0; // not received from any of the ports

              spi_txbuffer_set_sync_state(txbuf);
              spi_start_transfer(dma_tx, dma_rx, txbuf, rxbuf, dma_handler);
            } else {

              printf("%d\n",
                     spi_active_bucket
                         ->buffer[GRID_PARAMETER_SPI_SOURCE_FLAGS_index]);

              spi_txbuffer_set_sync_state(spi_active_bucket->buffer);
              spi_start_transfer(dma_tx, dma_rx, spi_active_bucket->buffer,
                                 rxbuf, dma_handler);
            }

          } else {
            // send empty packet with status flags

            txbuf[0] = 0;
            sprintf(txbuf, "DUMMY OK");
            txbuf[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = ready_flags;
            txbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] =
                0; // not received from any of the ports

            spi_txbuffer_set_sync_state(txbuf);
            spi_start_transfer(dma_tx, dma_rx, txbuf, rxbuf, dma_handler);
          }
        }
      }
    }

    loopcouter++;

    if (loopcouter > 500) {
      gpio_put(LED_PIN, 1);
    }
    if (loopcouter > 3000) {
      loopcouter = 0;
      gpio_put(LED_PIN, 0);
    }

    /* ==================================  UART TRANSMIT
     * =================================*/

    // iterate through all the ports
    uart_try_send_all();

    /* ==================================  UART RECEIVE
     * =================================*/

    // fifo_try_receive();
  }
}

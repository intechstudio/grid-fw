/**
 *
 * On-Board LED Blinky
 */

#include <stdio.h>
#include <stdlib.h>

#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/irq.h"
#include "hardware/spi.h"

#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#include "hardware/uart.h"

#include "pico/multicore.h"

#include "../../grid_common/grid_msg.h"
#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_protocol.h"

#include <string.h>

#include "pico/time.h"

#include "grid_pico_pins.h"
#include "grid_pico_platform.h"
#include "grid_pico_spi.h"

#include "hardware/dma.h"

#define BUCKET_BUFFER_LENGTH 500
#define BUCKET_ARRAY_LENGTH 50

struct grid_msg_recent_buffer recent_messages;

static uint8_t grid_pico_spi_txbuf[GRID_PARAMETER_SPI_TRANSACTION_length];
static uint8_t grid_pico_spi_rxbuf[GRID_PARAMETER_SPI_TRANSACTION_length];

uint8_t grid_pico_uart_tx_ready_bitmap = 255;

const PIO GRID_TX_PIO = pio0;
const PIO GRID_RX_PIO = pio1;

enum grid_bucket_status_t {

  GRID_BUCKET_STATUS_EMPTY,
  GRID_BUCKET_STATUS_RECEIVING,
  GRID_BUCKET_STATUS_RECEIVE_COMPLETED,
  GRID_BUCKET_STATUS_FULL_SEND_TO_SPI,
  GRID_BUCKET_STATUS_FULL_SEND_TO_NORTH,
  GRID_BUCKET_STATUS_FULL_SEND_TO_EAST,
  GRID_BUCKET_STATUS_FULL_SEND_TO_SOUTH,
  GRID_BUCKET_STATUS_FULL_SEND_TO_WEST,
  GRID_BUCKET_STATUS_TRANSMITTING,
  GRID_BUCKET_STATUS_DONE,

};

struct grid_bucket {

  enum grid_bucket_status_t status;
  uint8_t index;
  uint8_t source_port_index;
  uint8_t buffer[GRID_PARAMETER_SPI_TRANSACTION_length];
  uint16_t buffer_index;
};

// this is a doublebuffer
struct grid_pico_uart_port {

  uint8_t port_index;
  uint8_t tx_buffer[GRID_PARAMETER_SPI_TRANSACTION_length];

  struct grid_bucket* rx_bucket;

  struct grid_bucket* tx_active_bucket;
  struct grid_bucket* tx_lastinserted_bucket;
  struct grid_bucket* tx_previous_bucket;
};

uint8_t bucket_array_length = BUCKET_ARRAY_LENGTH;
struct grid_bucket bucket_array[BUCKET_ARRAY_LENGTH];

struct grid_pico_uart_port uart_port_array[4];

struct grid_pico_uart_port* uart_port_N = &uart_port_array[0];
struct grid_pico_uart_port* uart_port_E = &uart_port_array[1];
struct grid_pico_uart_port* uart_port_S = &uart_port_array[2];
struct grid_pico_uart_port* uart_port_W = &uart_port_array[3];

void grid_pico_uart_uart_port_init(struct grid_pico_uart_port* uart_port, uint8_t index) {

  uart_port->port_index = index;

  for (uint16_t i = 0; i < GRID_PARAMETER_SPI_TRANSACTION_length; i++) {
    uart_port->tx_buffer[i] = 0;
  }

  uart_port->rx_bucket = NULL;

  uart_port->tx_active_bucket = NULL;
  uart_port->tx_previous_bucket = NULL;
  uart_port->tx_lastinserted_bucket = NULL;
}

void grid_bucket_clear(struct grid_bucket* bucket) {

  bucket->status = GRID_BUCKET_STATUS_EMPTY;

  bucket->source_port_index = 255;

  // for (uint16_t i=0; i<GRID_PARAMETER_SPI_TRANSACTION_length; i++){
  //     bucket->buffer[i] = 0;
  // }

  bucket->buffer_index = 0;
}

void grid_bucket_init(struct grid_bucket* bucket, uint8_t index) {

  bucket->index = index;
  grid_bucket_clear(bucket);
}

// find next bucket, so UART can safely receive data into it
struct grid_bucket* grid_bucket_find_next_match(struct grid_bucket* previous_bucket, enum grid_bucket_status_t expected_status) {

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

struct grid_bucket* spi_tx_active_bucket = NULL;
struct grid_bucket* spi_rx_previous_bucket = NULL;

void grid_bucket_put_character(struct grid_bucket* bucket, char next_char) {

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

char grid_bucket_get_character(struct grid_bucket* bucket) {

  if (bucket == NULL) {
    return 0;
  }

  char c = bucket->buffer[bucket->buffer_index];
  bucket->buffer_index++;
  return c;
}

void grid_uart_port_attach_rx_bucket(struct grid_pico_uart_port* uart_port) {

  struct grid_bucket* bucket = grid_bucket_find_next_match(uart_port->rx_bucket, GRID_BUCKET_STATUS_EMPTY);

  if (bucket == NULL) {
    return;
  }

  uart_port->rx_bucket = bucket;

  uart_port->rx_bucket->status = GRID_BUCKET_STATUS_RECEIVING;
  uart_port->rx_bucket->source_port_index = uart_port->port_index;
}

void grid_uart_port_attach_tx_bucket(struct grid_pico_uart_port* uart_port) {

  struct grid_bucket* bucket = grid_bucket_find_next_match(uart_port->tx_previous_bucket, GRID_BUCKET_STATUS_FULL_SEND_TO_NORTH + uart_port->port_index);

  if (bucket == NULL) {
    // no more message, ready to receive more
    grid_pico_uart_tx_ready_bitmap |= (1 << uart_port->port_index);
    return;
  }

  bucket->buffer_index = 0;
  bucket->status = GRID_BUCKET_STATUS_TRANSMITTING;

  uart_port->tx_active_bucket = bucket;
}

void grid_uart_port_deattach_tx_bucket(struct grid_pico_uart_port* uart_port) {

  grid_bucket_clear(uart_port->tx_active_bucket);
  uart_port->tx_previous_bucket = uart_port->tx_active_bucket;
  uart_port->tx_active_bucket = NULL;
}

void grid_pico_uart_transmit_task_inner(struct grid_pico_uart_port* uart_port) {

  if (uart_port == NULL) {
    return;
  }

  if (uart_port->tx_active_bucket == NULL) {
    grid_uart_port_attach_tx_bucket(uart_port);
    return;
  }

  // if transmission is in progress then send the next character
  char c = grid_bucket_get_character(uart_port->tx_active_bucket);
  uart_tx_program_putc(GRID_TX_PIO, uart_port->port_index, c);

  if (c == '\n') {

    grid_uart_port_deattach_tx_bucket(uart_port);
  }
}

int grid_bucket_create_uart_tx_clone(struct grid_pico_uart_port* uart_port, struct grid_bucket* source_bucket) {

  if (uart_port == NULL) {
    return 1;
  }

  if (source_bucket == NULL) {
    return 1;
  }

  struct grid_bucket* bucket = grid_bucket_find_next_match(uart_port->tx_lastinserted_bucket, GRID_BUCKET_STATUS_EMPTY);

  if (bucket == NULL) {
    return 1;
  }

  strcpy(bucket->buffer, source_bucket->buffer);
  bucket->status = GRID_BUCKET_STATUS_FULL_SEND_TO_NORTH + uart_port->port_index;
}

int grid_uart_rx_process_bucket(struct grid_bucket* rx_bucket) {

  if (rx_bucket == NULL) {
    return 1;
  }

  char* message = (char*)rx_bucket->buffer;
  uint16_t length = strlen(message);

  int status = grid_str_verify_frame(message);

  if (status != 0) {
    grid_bucket_clear(rx_bucket);
    return 1;
  }

  struct grid_port* por = grid_transport_get_port(&grid_transport_state, rx_bucket->source_port_index);

  if (message[1] != GRID_CONST_BRC) {

    if (message[2] == GRID_CONST_BELL) {

      uint8_t error = 0;

      // reset timeout counter
      por->partner_last_timestamp = grid_platform_rtc_get_micros();

      if (por->partner_status == 0) {

        printf("C\n");

        // CONNECT
        por->partner_fi = (message[3] - por->direction + 6) % 4; // 0, 1, 2, 3 base on relative rotation of the modules
        por->partner_status = 1;
      }
    }

    rx_bucket->status = GRID_BUCKET_STATUS_FULL_SEND_TO_SPI;
    return 1;
  }

  grid_str_transform_brc_params(message, por->dx, por->dy, por->partner_fi); // update age, sx, sy, dx, dy, rot etc...

  // check if message is alreadys in recent messages buffer
  uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(message);
  if (grid_msg_recent_fingerprint_find(&recent_messages, fingerprint)) {
    // Already heard this message
    printf("H\r\n");
    grid_bucket_clear(rx_bucket);
    return 1;
  }

  grid_msg_recent_fingerprint_store(&recent_messages, fingerprint);

  // for (uint8_t i = 0; i < 4; i++) {
  //   struct grid_pico_uart_port* uart_port = &uart_port_array[i];
  //   if (uart_port->port_index != rx_bucket->source_port_index) {
  //     grid_bucket_create_uart_tx_clone(uart_port, rx_bucket);
  //     break;
  //   }

  // }

  // bucket content verified, close bucket and set it full to indicate that it is ready to be sent through to ESP32 via SPI
  rx_bucket->status = GRID_BUCKET_STATUS_FULL_SEND_TO_SPI;

  return 0;
}

void grid_pico_uart_port_receive_character(struct grid_pico_uart_port* uart_port, uint8_t character) {

  if (uart_port->rx_bucket == NULL) {
    grid_uart_port_attach_rx_bucket(uart_port);
  }

  if (character == GRID_CONST_SOH && uart_port->rx_bucket->buffer_index > 0) {
    // Start of Header character received in the middle of a transmission this typically happens when a partner is disconnected and reconnected
    printf("E\n");

    // clear the current active bucket, and attach to a new bucket to start
    // receiving packet
    grid_bucket_clear(uart_port->rx_bucket);

    grid_uart_port_attach_rx_bucket(uart_port);
  }

  grid_bucket_put_character(uart_port->rx_bucket, character);

  // printf("%c", character);

  if (character == '\n') {

    // end of message, put termination zero character
    grid_bucket_put_character(uart_port->rx_bucket, '\0');
    uart_port->rx_bucket->status = GRID_BUCKET_STATUS_RECEIVE_COMPLETED;

    // attack new bucket for receiving the next packet
    grid_uart_port_attach_rx_bucket(uart_port);
  }
}

void fifo_try_receive_characters(void) {

  uint32_t data = 0;
  uint32_t status = 0;
  // continue;

  while (multicore_fifo_rvalid()) {

    data = multicore_fifo_pop_blocking();

    // printf("POP");
    // direction is based on the position of the character in the uint32_t
    // i==0 is north...i==3 is west
    for (uint8_t i = 0; i < 4; i++) {

      uint8_t c = (data >> (8 * i)) & 0x000000FF;

      if (c == 0) {
        continue;
      }

      grid_pico_uart_port_receive_character(&uart_port_array[i], c);
    }
  }
}

void core_1_main_entry() {

  printf("Core 1 init\r\n");

  while (1) {

    uint32_t packed_chars = 0;

    // iterate through all the uart_ports and pack available characters
    for (uint8_t i = 0; i < 4; i++) {
      if (uart_rx_program_is_available(GRID_RX_PIO, i)) {

        char c = uart_rx_program_getc(GRID_RX_PIO, i);
        packed_chars |= (c << (8 * i));
      }
    }

    // try again if no characters were packed
    if (packed_chars == 0) {
      continue;
    }

    uint8_t ok = multicore_fifo_push_timeout_us(packed_chars, 1);

    if (!ok) {
      printf("F\n");
    }
  }
}

void core0_interrupt_handler(void) {

  fifo_try_receive_characters();
  multicore_fifo_clear_irq();
}

volatile uint8_t sync1_drive = 0;
volatile uint8_t sync1_interrupt = 0;
volatile uint8_t sync2_interrupt = 0;

void gpio_sync_pin_callback(uint gpio, uint32_t events) {

  if (gpio == GRID_PICO_PIN_SYNC1) {

    sync1_interrupt = 1;
  } else if (gpio == GRID_PICO_PIN_SYNC2) {

    sync2_interrupt = 1;
  }
}

void spi_uart_port_set_sync_state(uint8_t* buff) {

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

uint32_t grid_pico_get_time() { return time_us_32(); }

uint32_t grid_pico_get_elapsed_time(uint32_t t_old) { return time_us_32() - t_old; }

static uint32_t spi_receive_lastrealtime = 0;
static uint32_t spi_receive_interval_us = 100;

static uint32_t spi_transmit_lastrealtime = 0;
static uint32_t spi_transmit_interval_us = 500;

static uint32_t uart_receive_lastrealtime = 0;
static uint32_t uart_receive_interval_us = 50;

int spi_message_to_bucket(struct grid_pico_uart_port* uart_port, char* message) {

  if (uart_port == NULL) {
    return 1;
  }

  struct grid_bucket* bucket = grid_bucket_find_next_match(uart_port->tx_lastinserted_bucket, GRID_BUCKET_STATUS_EMPTY);

  if (bucket == NULL) {
    return 1;
  }

  for (uint16_t i = 0; i < GRID_PARAMETER_SPI_TRANSACTION_length; i++) {
    char c = grid_pico_spi_rxbuf[i];
    grid_bucket_put_character(bucket, c);

    if (c == '\n') {
      grid_bucket_put_character(bucket, '\0');
      break;
    }
  }

  bucket->status = GRID_BUCKET_STATUS_FULL_SEND_TO_NORTH + uart_port->port_index;

  // set bucket as last inserted
  uart_port->tx_lastinserted_bucket = bucket;

  grid_pico_uart_tx_ready_bitmap &= ~(1 << uart_port->port_index); // clear ready
}

void grid_pico_uart_receive_task_inner(void) {

  if (grid_pico_get_elapsed_time(uart_receive_lastrealtime) < uart_receive_interval_us) {
    return;
  }

  uart_receive_lastrealtime = grid_pico_get_time();

  struct grid_bucket* last_uart_rx_processed_bucket = NULL;
  struct grid_bucket* bucket = grid_bucket_find_next_match(last_uart_rx_processed_bucket, GRID_BUCKET_STATUS_RECEIVE_COMPLETED);
  grid_uart_rx_process_bucket(bucket);
}

void grid_pico_spi_receive_task_inner(void) {

  if (grid_pico_get_elapsed_time(spi_receive_lastrealtime) < spi_receive_interval_us) {
    return;
  }
  spi_receive_lastrealtime = grid_pico_get_time();

  if (!grid_pico_spi_is_rx_data_available()) {
    return;
  }
  grid_pico_spi_clear_rx_data_available_flag();

  uint8_t destination_flags = grid_pico_spi_rxbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];
  uint8_t sync1_state = grid_pico_spi_rxbuf[GRID_PARAMETER_SPI_SYNC1_STATE_index];

  if (sync1_state) {

    gpio_pull_down(GRID_PICO_PIN_SYNC1);
    sync1_drive = 1;
    gpio_set_dir(GRID_PICO_PIN_SYNC1, GPIO_OUT);
    gpio_put(GRID_PICO_PIN_SYNC1, 1);
    // set_drive_mode(GRID_PICO_PIN_SYNC1, GPIO_DRIVE_MODE_OPEN_DRAIN);
  } else if (sync1_drive) {
    gpio_set_dir(GRID_PICO_PIN_SYNC1, GPIO_IN);
    sync1_drive = 0;
  }

  // printf("%d\r\n", destination_flags);

  // iterate through all the uart_ports

  uint8_t index = __builtin_ctz(destination_flags);
  struct grid_pico_uart_port* uart_port = index < 4 ? &uart_port_array[index] : NULL;

  spi_message_to_bucket(uart_port, grid_pico_spi_rxbuf);

  if (spi_tx_active_bucket != NULL) {
    // clear the bucket after use
    grid_bucket_clear(spi_tx_active_bucket);
    spi_rx_previous_bucket = spi_tx_active_bucket;
    spi_tx_active_bucket = NULL;
  }
}

void grid_pico_spi_transmit_task_inner(void) {

  if (grid_pico_get_elapsed_time(spi_transmit_lastrealtime) < spi_transmit_interval_us) {
    return;
  }
  spi_transmit_lastrealtime = grid_pico_get_time();

  if (!grid_pico_spi_isready()) {
    return;
  }

  if (spi_tx_active_bucket != NULL) {
    // previous transfer's rx task has not run yet
    return;
  }

  // try to send bucket content through SPI

  spi_tx_active_bucket = grid_bucket_find_next_match(spi_rx_previous_bucket, GRID_BUCKET_STATUS_FULL_SEND_TO_SPI);

  if (spi_tx_active_bucket == NULL) {
    // No fully bucket is received from uart yet
    // send empty packet with status flags

    grid_pico_spi_txbuf[0] = 0;
    sprintf(grid_pico_spi_txbuf, "DUMMY OK");
    grid_pico_spi_txbuf[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = grid_pico_uart_tx_ready_bitmap;
    grid_pico_spi_txbuf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = 0; // not received from any of the ports

    spi_uart_port_set_sync_state(grid_pico_spi_txbuf);
    grid_pico_spi_transfer(grid_pico_spi_txbuf, grid_pico_spi_rxbuf);

    return;
  }

  // found full bucket, send it through SPI
  spi_tx_active_bucket->buffer_index = 0;
  // printf("SPI send: %s\r\n", spi_tx_active_bucket->buffer);

  spi_tx_active_bucket->buffer[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = grid_pico_uart_tx_ready_bitmap;
  spi_tx_active_bucket->buffer[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = (1 << (spi_tx_active_bucket->source_port_index));

  // printf("%d\n", spi_tx_active_bucket->buffer[GRID_PARAMETER_SPI_SOURCE_FLAGS_index]);

  spi_uart_port_set_sync_state(spi_tx_active_bucket->buffer);
  grid_pico_spi_transfer(spi_tx_active_bucket->buffer, grid_pico_spi_rxbuf);
}

int main() {

  set_sys_clock_khz(250000, true); // 2x overclock for RP2040. This is required because UART rx processing is not fast enough and I get lots of "F\n" in console otherwise
  stdio_init_all();
  uart_init(uart0, 2000000);

  printf("RP2040 START\r\n");
  printf("Build date and time: %s %s\n", __DATE__, __TIME__);

  grid_msg_recent_fingerprint_buffer_init(&recent_messages, 32);

  grid_transport_init(&grid_transport_state);
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_NORTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_EAST));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_SOUTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_WEST));

  uint offset_tx = pio_add_program(GRID_TX_PIO, &uart_tx_program);
  uint offset_rx = pio_add_program(GRID_RX_PIO, &uart_rx_program);

  // setup gpio sync interrupts
  gpio_set_irq_enabled_with_callback(GRID_PICO_PIN_SYNC1, GPIO_IRQ_EDGE_RISE, true, &gpio_sync_pin_callback);
  gpio_set_irq_enabled(GRID_PICO_PIN_SYNC2, GPIO_IRQ_EDGE_RISE, true);

  grid_pico_uart_uart_port_init(uart_port_N, 0);
  grid_pico_uart_uart_port_init(uart_port_E, 1);
  grid_pico_uart_uart_port_init(uart_port_S, 2);
  grid_pico_uart_uart_port_init(uart_port_W, 3);

  for (uint8_t i = 0; i < bucket_array_length; i++) {
    grid_bucket_init(&bucket_array[i], i);
  }

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_pico_uart_port* uart_port = &uart_port_array[i];

    grid_uart_port_attach_rx_bucket(uart_port);
  }

  uart_tx_program_init(GRID_TX_PIO, 0, offset_tx, GRID_PICO_PIN_NORTH_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 1, offset_tx, GRID_PICO_PIN_EAST_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 2, offset_tx, GRID_PICO_PIN_SOUTH_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 3, offset_tx, GRID_PICO_PIN_WEST_TX, GRID_PARAMETER_UART_baudrate);

  uart_rx_program_init(GRID_RX_PIO, 0, offset_rx, GRID_PICO_PIN_NORTH_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 1, offset_rx, GRID_PICO_PIN_EAST_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 2, offset_rx, GRID_PICO_PIN_SOUTH_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 3, offset_rx, GRID_PICO_PIN_WEST_RX, GRID_PARAMETER_UART_baudrate);

  gpio_init(GRID_PICO_PIN_SYNC1);
  gpio_init(GRID_PICO_PIN_SYNC2);

  // Setup COMMON stuff
  gpio_init(GRID_PICO_PIN_LED);
  gpio_set_dir(GRID_PICO_PIN_LED, GPIO_OUT);

  grid_pico_spi_init();

  for (uint i = 0; i < GRID_PARAMETER_SPI_TRANSACTION_length; i++) {
    grid_pico_spi_txbuf[i] = 0;
  }

  uint32_t loopcouter = 0;
  uint8_t spi_counter = 0;

  multicore_reset_core1();
  multicore_launch_core1(core_1_main_entry);

  multicore_fifo_clear_irq();
  irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_interrupt_handler);
  irq_set_enabled(SIO_IRQ_PROC0, true);

  while (1) {

    grid_pico_uart_receive_task_inner();

    grid_pico_spi_receive_task_inner();

    grid_pico_spi_transmit_task_inner();

    // iterate through all the uart_ports
    for (uint8_t i = 0; i < 4; i++) {
      grid_pico_uart_transmit_task_inner(&uart_port_array[i]);
    }

    for (uint8_t i = 0; i < 4; i++) {

      struct grid_port* por = grid_transport_get_port(&grid_transport_state, i);

      if (grid_port_should_uart_timeout_disconect_now(por)) { // try disconnect for uart port
        printf("D\n");
        por->partner_status = 0;
        // grid_port_receiver_softreset(por, doublebuffer_rx);
      }
    }

    loopcouter++;

    if (loopcouter % 1000 == 0) {
      gpio_put(GRID_PICO_PIN_LED, 1);
    } else {
      gpio_put(GRID_PICO_PIN_LED, 0);
    }
  }
}

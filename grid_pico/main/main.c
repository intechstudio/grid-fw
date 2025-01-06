#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/spi.h"

#include "grid_pico_spi.h"

#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

const PIO GRID_TX_PIO = pio0;
const PIO GRID_RX_PIO = pio1;

#include "grid_pico_pins.h"

#include "../../grid_common/grid_msg.h"
#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_protocol.h"

#include "vmp/vmp_def.h"
#include "vmp/vmp_tag.h"

#include "pico_pool.h"
#include "pico_swsr.h"

struct pico_pool_t pool;

uint32_t grid_pico_time() { return time_us_32(); }
uint32_t grid_pico_time_diff(uint32_t t1, uint32_t t2) { return t2 - t1; }

struct grid_pico_task_timer {
  uint32_t last;
  uint32_t period;
};

bool grid_pico_task_timer_elapsed(struct grid_pico_task_timer* timer) {

  uint32_t now = grid_pico_time();

  bool ret = grid_pico_time_diff(timer->last, now) >= timer->period;

  if (ret) {
    timer->last = now;
  }

  return ret;
}

enum {
  ROLLING_MAX = GRID_PARAMETER_SPI_ROLLING_ID_maximum,
};

struct grid_pico_rolling {
  uint8_t last_send;
  uint8_t last_recv;
  uint8_t errors;
};

volatile struct grid_pico_rolling rolling = (struct grid_pico_rolling){
    .last_send = UINT8_MAX,
    .last_recv = UINT8_MAX,
    .errors = 0,
};

void grid_pico_rolling_recv(volatile struct grid_pico_rolling* rolling, uint8_t recv) {

  uint8_t expect = (rolling->last_recv + 1) % ROLLING_MAX;

  if (recv != expect) {
    if (rolling->errors < UINT8_MAX) {
      ++rolling->errors;
    }
  }

  rolling->last_recv = recv;
}

void grid_pico_rolling_send(volatile struct grid_pico_rolling* rolling) { rolling->last_send = (rolling->last_send + 1) % ROLLING_MAX; }

struct grid_pico_uart_port {
  uint8_t index;
  uint8_t ready;
  struct pico_bkt_t* uart_tx_bucket;
  struct pico_bkt_t* uart_rx_bucket;
  struct pico_swsr_t swsr;
};

struct grid_pico_uart_port uart_ports[4];

int grid_pico_uart_port_malloc(struct grid_pico_uart_port* port) {

  if (!pico_swsr_malloc(&port->swsr, GRID_PARAMETER_SPI_TRANSACTION_length)) {
    return 1;
  }

  return 0;
}

void grid_pico_uart_port_init(struct grid_pico_uart_port* port, uint8_t index) {

  assert(index >= 0 && index < 4);

  port->index = index;
  port->ready = 1;
  port->uart_tx_bucket = NULL;
  port->uart_rx_bucket = NULL;
}

void grid_pico_uart_port_attach_rx(struct grid_pico_uart_port* port) {

  if (port->uart_rx_bucket) {
    return;
  }

  struct pico_bkt_t* bkt = pico_pool_get_first(&pool, PICO_BKT_STATE_EMPTY);

  if (!bkt) {
    return;
  }

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_RX_NORTH + port->index;
  bkt = pico_pool_change(&pool, bkt, state);

  if (!bkt) {
    return;
  }

  bkt->port = port->index;

  port->uart_rx_bucket = bkt;

  pico_bkt_reset(port->uart_rx_bucket);
}

void grid_pico_uart_port_detach_rx(struct grid_pico_uart_port* port) {

  if (!port->uart_rx_bucket) {
    return;
  }

  pico_bkt_reset(port->uart_rx_bucket);

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_RX_FULL_NORTH + port->index;
  pico_pool_change(&pool, port->uart_rx_bucket, state);

  port->uart_rx_bucket = NULL;
}

void grid_pico_uart_port_attach_tx(struct grid_pico_uart_port* port) {

  if (port->uart_tx_bucket) {
    return;
  }

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_TX_NORTH + port->index;
  struct pico_bkt_t* bkt = pico_pool_get_first(&pool, state);

  if (!bkt) {
    port->ready = 1;
    return;
  }

  port->ready = 0;

  port->uart_tx_bucket = bkt;

  pico_bkt_reset(port->uart_tx_bucket);
}

void grid_pico_uart_port_detach_tx(struct grid_pico_uart_port* port) {

  if (!port->uart_tx_bucket) {
    return;
  }

  pico_bkt_reset(port->uart_tx_bucket);

  enum pico_bkt_state_t state = PICO_BKT_STATE_EMPTY;
  pico_pool_change(&pool, port->uart_tx_bucket, state);

  port->uart_tx_bucket = NULL;
}

struct grid_pico_task_timer timer_uart_tx[4];

void grid_pico_task_uart_tx(struct grid_pico_uart_port* port, struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  if (!port->uart_tx_bucket) {
    grid_pico_uart_port_attach_tx(port);
    return;
  }

  char c = pico_bkt_next(port->uart_tx_bucket);
  uart_tx_program_putc(GRID_TX_PIO, port->index, c);

  if (c == '\n') {

    grid_pico_uart_port_detach_tx(port);
    // vmp_push(UART_TX);
  }
}

void grid_uart_rx_clone_tx(struct grid_pico_uart_port* dest_port, struct pico_bkt_t* src_bkt) {

  struct pico_bkt_t* bkt = pico_pool_get_first(&pool, PICO_BKT_STATE_EMPTY);

  if (!bkt) {
    return;
  }

  pico_bkt_reset(bkt);

  strcpy(bkt->buf, src_bkt->buf);

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_TX_NORTH + dest_port->index;
  pico_pool_change(&pool, bkt, state);
}

struct grid_msg_recent_buffer recent_msgs;

enum pico_bkt_state_t grid_uart_rx_process_bkt(struct grid_pico_uart_port* port, struct pico_bkt_t* bkt) {

  char* msg = (char*)bkt->buf;
  uint16_t len = strlen(msg);

  if (len < 14) {
    return PICO_BKT_STATE_EMPTY;
  }

  if (bkt->port > 3) {
    return PICO_BKT_STATE_EMPTY;
  }

  int status = grid_str_verify_frame(msg);

  if (status != 0) {
    return PICO_BKT_STATE_EMPTY;
  }

  struct grid_port* por = grid_transport_get_port(&grid_transport_state, port->index);

  if (!por) {
    return PICO_BKT_STATE_EMPTY;
  }

  if (msg[1] != GRID_CONST_BRC) {

    if (msg[2] == GRID_CONST_BELL) {

      por->partner_fi = (msg[3] - por->direction + 6) % 4;
      por->partner_status = 1;

      return PICO_BKT_STATE_SPI_TX;
    }

    return PICO_BKT_STATE_EMPTY;
  }

  grid_str_transform_brc_params(msg, por->dx, por->dy, por->partner_fi);

  uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(msg);

  if (grid_msg_recent_fingerprint_find(&recent_msgs, fingerprint)) {
    return PICO_BKT_STATE_EMPTY;
  }

  grid_msg_recent_fingerprint_store(&recent_msgs, fingerprint);

  for (int i = 0; i < 4; ++i) {

    struct grid_pico_uart_port* target = &uart_ports[i];

    if (port->index != target->index) {
      grid_uart_rx_clone_tx(target, bkt);
    }
  }

  return PICO_BKT_STATE_SPI_TX;
}

struct grid_pico_task_timer timer_uart_rx_full[4];

void grid_pico_task_uart_rx_full(struct grid_pico_uart_port* port, struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_RX_FULL_NORTH + port->index;
  struct pico_bkt_t* bkt = pico_pool_get_first(&pool, state);

  if (!bkt) {
    return;
  }

  enum pico_bkt_state_t next_state = grid_uart_rx_process_bkt(port, bkt);

  pico_bkt_reset(bkt);

  pico_pool_change(&pool, bkt, next_state);
}

//

static uint8_t spi_tx_buf[GRID_PARAMETER_SPI_TRANSACTION_length];
static uint8_t spi_rx_buf[GRID_PARAMETER_SPI_TRANSACTION_length];

struct pico_bkt_t* spi_tx_bucket = NULL;

void grid_spi_msg_to_bkt(struct grid_pico_uart_port* port, char* msg) {

  struct pico_bkt_t* bkt = pico_pool_get_first(&pool, PICO_BKT_STATE_EMPTY);

  if (!bkt) {
    return;
  }

  pico_bkt_reset(bkt);

  size_t len = strnlen(msg, GRID_PARAMETER_SPI_TRANSACTION_length - 1);

  memcpy(bkt->buf, msg, len);
  bkt->buf[len] = '\0';

  enum pico_bkt_state_t state = PICO_BKT_STATE_UART_TX_NORTH + port->index;
  pico_pool_change(&pool, bkt, state);

  // port->ready = 0;
}

struct grid_pico_task_timer timer_spi_rx;

void grid_pico_task_spi_rx(struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  if (!grid_pico_spi_is_rx_data_available()) {
    return;
  }

  // vmp_push(SPI_RX);

  grid_pico_spi_clear_rx_data_available_flag();

  // Receive rolling ID
  uint8_t rolling_recv = spi_rx_buf[GRID_PARAMETER_SPI_ROLLING_ID_index];
  grid_pico_rolling_recv(&rolling, rolling_recv);

  // Control LCD backlight
  gpio_put(GRID_PICO_LCD_BACKLIGHT_PIN, spi_rx_buf[GRID_PARAMETER_SPI_BACKLIGHT_PWM_index]);

  // The number of trailing zeroes in the destination flag
  // indexes the destination UART port of the message
  uint8_t dest_flags = spi_rx_buf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];
  uint8_t tz = __builtin_ctz(dest_flags);
  struct grid_pico_uart_port* uart_port = tz < 4 ? &uart_ports[tz] : NULL;

  // If the destination UART port is valid
  if (uart_port) {

    // Put the message to be sent out into a bucket
    grid_spi_msg_to_bkt(uart_port, spi_rx_buf);
  }

  // Reset any attached tx bucket and mark it for reuse after use,
  // this arbitrary bit of logic is specific to the SPI RX/TX task pair
  if (spi_tx_bucket) {

    pico_bkt_reset(spi_tx_bucket);
    pico_pool_change(&pool, spi_tx_bucket, PICO_BKT_STATE_EMPTY);

    spi_tx_bucket = NULL;
  }
}

struct grid_pico_task_timer timer_spi_tx;

void grid_pico_task_spi_tx(struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  if (!grid_pico_spi_isready()) {
    return;
  }

  // Check that the previous transfer's rx task has reset the bucket
  if (spi_tx_bucket) {
    return;
  }

  // Attempt to attach next bucket to be sent through SPI
  enum pico_bkt_state_t state = PICO_BKT_STATE_SPI_TX;
  spi_tx_bucket = pico_pool_get_first(&pool, state);

  // vmp_push(SPI_TX);

  // Construct the bitfield containing which ports are ready to transmit
  uint8_t tx_ready = 0;
  for (int i = 0; i < 4; ++i) {
    tx_ready |= (uart_ports[i].ready != 0) << uart_ports[i].index;
  }

  // Increment rolling ID
  grid_pico_rolling_send(&rolling);

  // No full bucket received from UART yet
  if (!spi_tx_bucket) {

    // Send empty packet with status flags,
    // to convince the main processor of our presence
    sprintf(spi_tx_buf, "DUMMY OK");
    spi_tx_buf[GRID_PARAMETER_SPI_ROLLING_ID_index] = rolling.last_send;
    spi_tx_buf[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = tx_ready;
    spi_tx_buf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = 0; // no origin port

    grid_pico_spi_transfer(spi_tx_buf, spi_rx_buf);
  }
  // Found full bucket to be sent through SPI
  else {

    // pico_bkt_reset(spi_tx_bucket);

    uint8_t* buf = spi_tx_bucket->buf;
    buf[GRID_PARAMETER_SPI_ROLLING_ID_index] = rolling.last_send;
    buf[GRID_PARAMETER_SPI_STATUS_FLAGS_index] = tx_ready;
    buf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = 1 << spi_tx_bucket->port;

    grid_pico_spi_transfer(buf, spi_rx_buf);
  }
}

void grid_pico_uart_port_rx_char(struct grid_pico_uart_port* port, char ch) {

  if (!port->uart_rx_bucket) {
    grid_pico_uart_port_attach_rx(port);
  }

  if (!port->uart_rx_bucket) {
    return;
  }

  // Receiving start-of-header in the middle of a transmission,
  // which typically happens after disconnection, then reconnection
  if (ch == GRID_CONST_SOH && port->uart_rx_bucket->index > 0) {

    pico_bkt_reset(port->uart_rx_bucket);
  }

  pico_bkt_push(port->uart_rx_bucket, ch);

  if (ch == '\n') {

    // End of message, requires null termination
    pico_bkt_push(port->uart_rx_bucket, '\0');

    grid_pico_uart_port_detach_rx(port);

    // vmp_push(UART_RX);
  }
}

struct grid_pico_task_timer timer_uart_rx_0[4];

void grid_pico_task_uart_rx_0(struct grid_pico_uart_port* port, struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  if (!pico_swsr_readable(&port->swsr)) {
    return;
  }

  char c = pico_swsr_read(&port->swsr);
  grid_pico_uart_port_rx_char(port, c);
}

struct grid_pico_task_timer timer_uart_rx_1[4];

void grid_pico_task_uart_rx_1(struct grid_pico_uart_port* port, struct grid_pico_task_timer* timer) {

  if (!grid_pico_task_timer_elapsed(timer)) {
    return;
  }

  if (!uart_rx_program_is_available(GRID_RX_PIO, port->index)) {
    return;
  }

  if (!pico_swsr_writable(&port->swsr)) {
    return;
  }

  char c = uart_rx_program_getc(GRID_RX_PIO, port->index);
  pico_swsr_write(&port->swsr, c);
}

void core_1_main_entry() {

  // Configure task timers
  for (int i = 0; i < 4; ++i) {
    timer_uart_rx_1[i] = (struct grid_pico_task_timer){
        .last = grid_pico_time(),
        .period = 5,
    };
  }

  while (1) {

    for (int i = 0; i < 4; ++i) {
      grid_pico_task_uart_rx_1(&uart_ports[i], &timer_uart_rx_1[i]);
    }
  }
}

int main() {

  stdio_init_all();

  grid_pico_spi_init();

  uart_init(uart0, 2000000);

  // Reset and launch second core
  multicore_reset_core1();
  multicore_launch_core1(core_1_main_entry);

  // Initialize fingerprint buffer
  grid_msg_recent_fingerprint_buffer_init(&recent_msgs, 32);

  // Initialize transport, used for some protocol mechanisms
  // such as keeping track of delta offsets and rotation
  grid_transport_init(&grid_transport_state);
  printf("grid_transport_register_port ALL\n");
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_NORTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_EAST));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_SOUTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_WEST));

  // Initialize UART ports
  for (int i = 0; i < 4; ++i) {
    grid_pico_uart_port_malloc(&uart_ports[i]);
  }
  grid_pico_uart_port_init(&uart_ports[0], 0);
  grid_pico_uart_port_init(&uart_ports[1], 1);
  grid_pico_uart_port_init(&uart_ports[2], 2);
  grid_pico_uart_port_init(&uart_ports[3], 3);

  int offset_tx = pio_add_program(GRID_TX_PIO, &uart_tx_program);
  int offset_rx = pio_add_program(GRID_RX_PIO, &uart_rx_program);

  uart_tx_program_init(GRID_TX_PIO, 0, offset_tx, GRID_PICO_PIN_NORTH_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 1, offset_tx, GRID_PICO_PIN_EAST_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 2, offset_tx, GRID_PICO_PIN_SOUTH_TX, GRID_PARAMETER_UART_baudrate);
  uart_tx_program_init(GRID_TX_PIO, 3, offset_tx, GRID_PICO_PIN_WEST_TX, GRID_PARAMETER_UART_baudrate);

  uart_rx_program_init(GRID_RX_PIO, 0, offset_rx, GRID_PICO_PIN_NORTH_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 1, offset_rx, GRID_PICO_PIN_EAST_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 2, offset_rx, GRID_PICO_PIN_SOUTH_RX, GRID_PARAMETER_UART_baudrate);
  uart_rx_program_init(GRID_RX_PIO, 3, offset_rx, GRID_PICO_PIN_WEST_RX, GRID_PARAMETER_UART_baudrate);

  // Initialize bucket pool
  pico_pool_init(&pool);

  // Configure task timers
  for (int i = 0; i < 4; ++i) {
    timer_uart_tx[i] = (struct grid_pico_task_timer){
        .last = grid_pico_time(),
        .period = 5,
    };
    timer_uart_rx_full[i] = (struct grid_pico_task_timer){
        .last = grid_pico_time(),
        .period = 5,
    };
    timer_uart_rx_0[i] = (struct grid_pico_task_timer){
        .last = grid_pico_time(),
        .period = 5,
    };
  }
  timer_spi_rx = (struct grid_pico_task_timer){
      .last = grid_pico_time(),
      .period = 100,
  };
  timer_spi_tx = (struct grid_pico_task_timer){
      .last = grid_pico_time(),
      .period = 500,
  };

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };

  // bool vmp_flushed = false;
  while (1) {

    // vmp_push(MAIN);

    for (int i = 0; i < 4; ++i) {
      grid_pico_task_uart_tx(&uart_ports[i], &timer_uart_tx[i]);
      grid_pico_task_uart_rx_0(&uart_ports[i], &timer_uart_rx_0[i]);
      grid_pico_task_uart_rx_full(&uart_ports[i], &timer_uart_rx_full[i]);
    }
    grid_pico_task_spi_rx(&timer_spi_rx);
    grid_pico_task_spi_tx(&timer_spi_tx);

    /*
    if (!vmp_flushed && vmp.size == vmp.capacity) {

      vmp_serialize_start(&reg);
      vmp_buf_serialize_and_write(&vmp, &reg);
      vmp_uid_str_serialize_and_write(VMP_UID_COUNT, VMP_ASSOC, &reg);
      vmp_serialize_close(&reg);

      vmp_buf_free(&vmp);

      vmp_flushed = true;
    }
    */
  }
}

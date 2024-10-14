/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_port.h"

#include "esp_check.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

#include "esp_freertos_hooks.h"

#include "driver/gpio.h"
#include "tinyusb.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "grid_buf.h"
#include "grid_esp32_pins.h"
#include "grid_esp32_platform.h"
#include "grid_port.h"
#include "grid_sys.h"

#include "driver/spi_slave.h"

extern uint32_t grid_platform_get_cycles(void);
struct grid_msg_recent_buffer DRAM_ATTR recent_messages;

static TaskHandle_t xTaskToNotify = NULL;

static const char* TAG = "PORT";

#define RCV_HOST SPI2_HOST

volatile uint8_t DRAM_ATTR rolling_id_last_sent = 255;
volatile uint8_t DRAM_ATTR rolling_id_last_received = 255;

volatile uint8_t DRAM_ATTR rolling_id_error_count = 0;

volatile uint8_t DRAM_ATTR status_ui_error_count = 0;
volatile uint8_t DRAM_ATTR status_host_error_count = 0;

volatile uint8_t DRAM_ATTR invalid_partner_status_count = 0;

uint8_t DRAM_ATTR empty_tx_buffer[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};
uint8_t DRAM_ATTR message_tx_buffer[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

spi_slave_transaction_t DRAM_ATTR outbnound_transaction[4];
spi_slave_transaction_t DRAM_ATTR spi_empty_transaction;

uint8_t DRAM_ATTR queue_state = 0;
uint8_t DRAM_ATTR sync1_received = 0;
uint8_t DRAM_ATTR sync2_received = 0;
uint8_t DRAM_ATTR sync1_state = 0;

void grid_platform_sync1_pulse_send() { sync1_state++; }

SemaphoreHandle_t queue_state_sem;
SemaphoreHandle_t spi_ready_sem;

void ets_debug_string(char* tag, char* str) {

  uint16_t length = strlen(str);

  ets_printf("%s: ", tag);
  for (uint8_t i = 0; i < length; i++) {

    if (str[i] < 32) {

      ets_printf("[%x] ", str[i]);
    } else {
      ets_printf("%c ", str[i]);
    }
  }
  ets_printf("\r\n");
};

static void IRAM_ATTR my_post_setup_cb(spi_slave_transaction_t* trans) {
  // printf("$\r\n");

  portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&spinlock);

  ((uint8_t*)trans->tx_buffer)[GRID_PARAMETER_SPI_SYNC1_STATE_index] = 0;

  if (sync1_state) {

    ((uint8_t*)trans->tx_buffer)[GRID_PARAMETER_SPI_SYNC1_STATE_index] = sync1_state;
    sync1_state--;
  }

  rolling_id_last_sent = (rolling_id_last_sent + 1) % GRID_PARAMETER_SPI_ROLLING_ID_maximum;
  ((uint8_t*)trans->tx_buffer)[GRID_PARAMETER_SPI_ROLLING_ID_index] = rolling_id_last_sent; // not received from any of the ports

  portEXIT_CRITICAL(&spinlock);
}

static char DRAM_ATTR rx_str[500] = {0};

static struct grid_port* DRAM_ATTR uart_port_array[4] = {0};
static struct grid_buffer* DRAM_ATTR ui_buffer_tx = NULL;
static struct grid_buffer* DRAM_ATTR host_buffer_tx = NULL;
static struct grid_buffer* DRAM_ATTR uart_buffer_tx_array[4] = {0};
static struct grid_buffer* DRAM_ATTR uart_buffer_rx_array[4] = {0};
static struct grid_doublebuffer* DRAM_ATTR uart_doublebuffer_tx_array[4] = {0};

static void IRAM_ATTR my_post_trans_cb(spi_slave_transaction_t* trans) {

  uint8_t rolling_id_now_received = ((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_ROLLING_ID_index];
  // ets_printf("ID: %d\n",  rolling_id_now_received);
  if (rolling_id_now_received != (rolling_id_last_received + 1) % GRID_PARAMETER_SPI_ROLLING_ID_maximum) {
    // ets_printf("ERROR: %d != %d\r\n", rolling_id_now_received, rolling_id_last_received+1);
    // ets_debug_string("STR: ", (char*)trans->rx_buffer);
    if (rolling_id_error_count < 255) {
      rolling_id_error_count++;
    }
  }
  rolling_id_last_received = rolling_id_now_received;

  if (((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index]) {
    strcpy(rx_str, (char*)trans->rx_buffer);
  }

  portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&spinlock);

  if (((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_SYNC1_STATE_index]) {
    sync1_received++;
  }
  if (((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_SYNC2_STATE_index]) {
    sync2_received++;
  }

  if (queue_state > 0) {
    queue_state--;
  }

  if (queue_state == 0) {

    queue_state++;
    ESP_ERROR_CHECK(spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, 0));
  }

  portEXIT_CRITICAL(&spinlock);

  uint8_t grid_pico_uart_tx_ready_bitmap = ((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_STATUS_FLAGS_index];

  for (uint8_t i = 0; i < 4; i++) {
    struct grid_port* por = uart_port_array[i];
    struct grid_buffer* buffer_tx = uart_buffer_tx_array[i];
    struct grid_buffer* buffer_rx = uart_buffer_rx_array[i];
    struct grid_doublebuffer* doublebuffer_tx = uart_doublebuffer_tx_array[i];

    if ((grid_pico_uart_tx_ready_bitmap & (0b00000001 << i))) {

      if (doublebuffer_tx->status == UINT16_MAX) { // magic constant

        doublebuffer_tx->status = 0;
      } else if (doublebuffer_tx->status > 0) {

        doublebuffer_tx->status = UINT16_MAX; // magic constant
      }
    }
  }

  uint8_t source_flags = ((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];
  uint8_t index = __builtin_ctz(source_flags); // count trailing zeros

  struct grid_port* por = por = index < 4 ? uart_port_array[index] : NULL;

  if (por == NULL) {
    // no message to copy to rx_double_buffer
    return;
  }

  // grid_port_receive_decode(por, &recent_messages, trans->rx_buffer, strlen(trans->rx_buffer));

  char* message = (char*)trans->rx_buffer;
  uint16_t length = strlen(trans->rx_buffer);

  // if (0 != grid_str_verify_frame(message)) {
  //   // message has invalid fram or checksum, cannot be decoded safely
  //   return;
  // }

  if (message[1] == GRID_CONST_BRC) { // Broadcast message

    if (por->partner_status == 0) {
      // dont allow message to reach rx_buffer if we are not connected because partner_phi based transform will be invalid!
      invalid_partner_status_count++;
      return;
    }

    // struct grid_buffer* rx_buffer = uart_buffer_rx_array[por->index];
    // grid_buffer_write_from_chunk(rx_buffer, message, length);

    int status_ui = grid_buffer_write_from_chunk(ui_buffer_tx, message, length);
    int status_host = grid_buffer_write_from_chunk(host_buffer_tx, message, length);

    if (status_ui == 0) {
      status_ui_error_count++;
    }

    if (status_host == 0) {
      status_host_error_count++;
    }

  } else if (message[1] == GRID_CONST_DCT) { // Direct Message

    if (message[2] == GRID_CONST_BELL) {

      uint8_t error = 0;

      // reset timeout counter
      por->partner_last_timestamp = grid_platform_rtc_get_micros();

      por->partner_fi = (message[3] - por->direction + 6) % 4; // 0, 1, 2, 3 base on relative rotation of the modules
      por->partner_status = 1;
    }
  }
}

static uint64_t cooldown_lastrealtime = 0;
static uint64_t ping_lastrealtime = 0;
static uint64_t heartbeat_lastrealtime = 0;

static void try_send_ping(void) {

  if (grid_platform_rtc_get_elapsed_time(ping_lastrealtime) < GRID_PARAMETER_PINGINTERVAL_us) {
    return;
  }

  ping_lastrealtime = grid_platform_rtc_get_micros();

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);

    if (port == NULL) {
      return; // no ports initialized
    }

    port->ping_flag = 1;
  }

  grid_port_ping_try_everywhere();
}

static void try_send_heartbeat(void) {

  if (grid_platform_rtc_get_elapsed_time(heartbeat_lastrealtime) < GRID_PARAMETER_HEARTBEATINTERVAL_us) {
    return;
  }

  heartbeat_lastrealtime = grid_platform_rtc_get_micros();
  grid_protocol_send_heartbeat(grid_msg_get_heartbeat_type(&grid_msg_state), grid_sys_get_hwcfg(&grid_sys_state)); // Put heartbeat into UI rx_buffer
}

void process_ui_local(void) {

  if (0 == grid_ui_event_count_istriggered_local(&grid_ui_state)) {
    return; // no local trigger found
  }

  // CRITICAL_SECTION_ENTER()
  vTaskSuspendAll();
  grid_port_process_ui_local_UNSAFE(&grid_ui_state);
  xTaskResumeAll();
  // CRITICAL_SECTION_LEAVE()
}

void process_ui_normal(void) {

  if (0 < grid_ui_event_count_istriggered_local(&grid_ui_state)) {
    return; // local trigger found, should service that first
  }

  if (0 == grid_ui_event_count_istriggered(&grid_ui_state)) {
    return; // no normal trigger found
  }

  if (grid_platform_rtc_get_elapsed_time(cooldown_lastrealtime) < GRID_PARAMETER_UICOOLDOWN_us) {
    return; // cooldown is not completed
  }

  cooldown_lastrealtime = grid_platform_rtc_get_micros();

  // CRITICAL_SECTION_ENTER()
  vTaskSuspendAll();
  grid_port_process_ui_UNSAFE(&grid_ui_state);
  xTaskResumeAll();
  // CRITICAL_SECTION_LEAVE()
}

static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length) {

  // grid_platform_printf("-> %d ", length);

  uint8_t dir_index = direction - GRID_CONST_NORTH;

  spi_slave_transaction_t* t = &outbnound_transaction[dir_index];

  ((uint8_t*)t->tx_buffer)[length] = 0; // termination zero after the message
  ((uint8_t*)t->tx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = (1 << dir_index);

  // ets_printf("%02x %02x %02x %02x ... len: %d\r\n",
  // ((uint8_t*)t->tx_buffer)[0], ((uint8_t*)t->tx_buffer)[1],
  // ((uint8_t*)t->tx_buffer)[2], ((uint8_t*)t->tx_buffer)[3], length);

  ////ets_printf("SEND %d: %s\r\n", dir_index, buffer);
  // ets_printf("#");

  portENTER_CRITICAL(&spinlock);

  esp_err_t ret = spi_slave_queue_trans(RCV_HOST, t, 0);
  if (ret == ESP_OK) {
    queue_state++;
  } else {
    // Some outgoing packet could not be transmitted!!!

    // while(1){
    //     ets_printf("TRAP %d\r\n", queue_state);
    //     ets_delay_us(1000*1000);
    // }
  }

  portEXIT_CRITICAL(&spinlock);

  // ets_printf("!");

  return 0; // done
}

static void plot_port_debug() {

  uint16_t plot[20] = {0};

  uint8_t port_list_length = grid_transport_get_port_array_length(&grid_transport_state);

  for (uint8_t i = 0; i < port_list_length; i++) {

    struct grid_buffer* tx_buffer = grid_transport_get_buffer_tx(&grid_transport_state, i);
    struct grid_buffer* rx_buffer = grid_transport_get_buffer_rx(&grid_transport_state, i);

    plot[i + 0] = grid_buffer_get_space(tx_buffer);
    plot[i + 6] = grid_buffer_get_space(rx_buffer);
  }

  for (uint8_t i = 0; i < 12; i++) {

    if (i == 0) {
      // ets_printf("TX: ");
    } else if (i == 4) {

      // ets_printf("|");
    } else if (i == 6) {

      // ets_printf(" RX: ");
    } else if (i == 10) {

      // ets_printf("|");
    }

    uint8_t value = (2000 - plot[i]) / 20;
    switch (value) {
    case 0:  // ets_printf(" "); break;
    case 1:  // ets_printf("▁"); break;
    case 2:  // ets_printf("▂"); break;
    case 3:  // ets_printf("▃"); break;
    case 4:  // ets_printf("▄"); break;
    case 5:  // ets_printf("▅"); break;
    case 6:  // ets_printf("▆"); break;
    case 7:  // ets_printf("▇"); break;
    case 8:  // ets_printf("█"); break;
    case 9:  // ets_printf("#"); break;
    default: // ets_printf("@"); break;
    }

    // ets_printf(" ");
  }

  // ets_printf("\r\n");
}

void handle_sync_ticks(void) {

  portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&spinlock);

  while (sync1_received) {
    grid_ui_midi_sync_tick_time(&grid_ui_state);
    sync1_received--;
  }

  while (sync2_received) {
    grid_ui_midi_sync_tick_time(&grid_ui_state);
    sync2_received--;
  }

  portEXIT_CRITICAL(&spinlock);
}

void handle_connect_disconnect_effect(uint8_t* partner_last_status) {
  for (uint8_t i = 0; i < 4; i++) {
    struct grid_port* por = grid_transport_get_port(&grid_transport_state, i);

    if (por->type != GRID_PORT_TYPE_USART) {
      abort();
    }

    if (partner_last_status[i] < por->partner_status) { // ONLY USE FOR LED EFFECT
      // connect
      partner_last_status[i] = 1;
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
    } else if (partner_last_status[i] > por->partner_status) { // ONLY USE FOR LED EFFECT

      // Disconnect
      partner_last_status[i] = 0;
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
    }

    if (grid_port_should_uart_timeout_disconect_now(por)) { // try disconnect for uart port
      por->partner_status = 0;
      grid_port_receiver_softreset(por, NULL);
    }
  }
}

void grid_esp32_port_task(void* arg) {

  struct grid_port* ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);
  struct grid_port* host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);

  ui_buffer_tx = grid_transport_get_buffer_tx(ui_port->parent, ui_port->index);
  host_buffer_tx = grid_transport_get_buffer_tx(host_port->parent, host_port->index);

  uart_port_array[0] = grid_transport_get_port(&grid_transport_state, 0);
  uart_port_array[1] = grid_transport_get_port(&grid_transport_state, 1);
  uart_port_array[2] = grid_transport_get_port(&grid_transport_state, 2);
  uart_port_array[3] = grid_transport_get_port(&grid_transport_state, 3);

  uart_buffer_tx_array[0] = grid_transport_get_buffer_tx(&grid_transport_state, 0);
  uart_buffer_tx_array[1] = grid_transport_get_buffer_tx(&grid_transport_state, 1);
  uart_buffer_tx_array[2] = grid_transport_get_buffer_tx(&grid_transport_state, 2);
  uart_buffer_tx_array[3] = grid_transport_get_buffer_tx(&grid_transport_state, 3);

  uart_buffer_rx_array[0] = grid_transport_get_buffer_rx(&grid_transport_state, 0);
  uart_buffer_rx_array[1] = grid_transport_get_buffer_rx(&grid_transport_state, 1);
  uart_buffer_rx_array[2] = grid_transport_get_buffer_rx(&grid_transport_state, 2);
  uart_buffer_rx_array[3] = grid_transport_get_buffer_rx(&grid_transport_state, 3);

  uart_doublebuffer_tx_array[0] = grid_transport_get_doublebuffer_tx(&grid_transport_state, 0);
  uart_doublebuffer_tx_array[1] = grid_transport_get_doublebuffer_tx(&grid_transport_state, 1);
  uart_doublebuffer_tx_array[2] = grid_transport_get_doublebuffer_tx(&grid_transport_state, 2);
  uart_doublebuffer_tx_array[3] = grid_transport_get_doublebuffer_tx(&grid_transport_state, 3);

  uint8_t n = 0;
  esp_err_t ret;

  // Configuration for the SPI bus
  spi_bus_config_t buscfg = {.mosi_io_num = GRID_ESP32_PINS_RP_MOSI,
                             .miso_io_num = GRID_ESP32_PINS_RP_MISO,
                             .sclk_io_num = GRID_ESP32_PINS_RP_SCLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .intr_flags = ESP_INTR_FLAG_IRAM};

  // Configuration for the SPI slave interface
  spi_slave_interface_config_t slvcfg = {.mode = 0, .spics_io_num = GRID_ESP32_PINS_RP_CS, .queue_size = 6, .flags = 0, .post_setup_cb = my_post_setup_cb, .post_trans_cb = my_post_trans_cb};

  // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master
  // is connected.
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_CS, GPIO_PULLUP_ONLY);

  // Initialize SPI slave interface
  ret = spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
  assert(ret == ESP_OK);

  WORD_ALIGNED_ATTR char sendbuf[GRID_PARAMETER_SPI_TRANSACTION_length + 1] = {0};
  WORD_ALIGNED_ATTR char recvbuf[GRID_PARAMETER_SPI_TRANSACTION_length + 1] = {0};

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);
    struct grid_doublebuffer* doublebuffer_tx = uart_doublebuffer_tx_array[i];

    // Set up a transaction of GRID_PARAMETER_SPI_TRANSACTION_length bytes to
    // send/receive

    memset(&outbnound_transaction[i], 0, sizeof(outbnound_transaction[i]));
    outbnound_transaction[i].length = GRID_PARAMETER_SPI_TRANSACTION_length * 8;
    outbnound_transaction[i].tx_buffer = doublebuffer_tx->buffer_storage;
    outbnound_transaction[i].rx_buffer = recvbuf;
  }

  empty_tx_buffer[0] = 'X';

  spi_empty_transaction.length = GRID_PARAMETER_SPI_TRANSACTION_length * 8;
  spi_empty_transaction.tx_buffer = empty_tx_buffer;
  spi_empty_transaction.rx_buffer = recvbuf;

  SemaphoreHandle_t spi_ready_sem = xSemaphoreCreateBinary();

  portENTER_CRITICAL(&spinlock);
  ret = spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, 0);
  ESP_ERROR_CHECK(ret);
  if (ret == ESP_OK) {
    queue_state++;
  }
  portEXIT_CRITICAL(&spinlock);

  uint8_t firstprint = 1;

  // gpio_set_direction(47, GPIO_MODE_OUTPUT);

  // partner_last_status array holds the last state. This is used for checking changes and triggering led effects accordingly
  uint8_t partner_last_status[grid_transport_get_port_array_length(&grid_transport_state)];
  memset(partner_last_status, 0, grid_transport_get_port_array_length(&grid_transport_state));

  grid_msg_recent_fingerprint_buffer_init(&recent_messages, 32);

  uint8_t watchdog_rolling_id_last_received = rolling_id_last_received;
  uint64_t watchdog_rolling_id_lastrealtime = grid_platform_rtc_get_micros();

  while (1) {

    for (uint8_t i = 0; i < 2; i++) {

      struct grid_buffer* buf = host_buffer_tx;

      if (i == 0) {

        buf = host_buffer_tx;
      } else {
        buf = ui_buffer_tx;
      }

      if (buf->write_init_error_count > 0) {
        buf->write_init_error_count = 0;
        ets_printf("ERROR: GRID BUFFER WRITE INIT %d\n", i);
      }

      if (buf->write_acknowledge_error_count > 0) {
        buf->write_acknowledge_error_count = 0;
        ets_printf("ERROR: GRID BUFFER WRITE ACKNOWLEDGE %d\n", i);
      }
    }

    if (invalid_partner_status_count > 0) {
      invalid_partner_status_count = 0;
      ets_printf("ERROR: INVALID PARTNER STATUS\n");
    }

    if (status_ui_error_count > 0) {
      status_ui_error_count = 0;
      ets_printf("ERROR: UI BUFFER\n");
    }

    if (status_host_error_count > 0) {
      status_host_error_count = 0;
      ets_printf("ERROR: HOST BUFFER\n");
    }

    if (rolling_id_error_count > 0) {
      rolling_id_error_count = 0;
      ets_printf("ERROR: Rolling ID\n");
    }

    if (rolling_id_last_received != watchdog_rolling_id_last_received) {
      watchdog_rolling_id_lastrealtime = grid_platform_rtc_get_micros();
      watchdog_rolling_id_last_received = rolling_id_last_received;
    }

    if (grid_platform_rtc_get_elapsed_time(watchdog_rolling_id_lastrealtime) > 100 * 1000) { // 100 ms

      watchdog_rolling_id_lastrealtime = grid_platform_rtc_get_micros();
      ets_printf("ERROR: SPI frozen\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 50);
      grid_alert_all_set_frequency(&grid_led_state, -4);
      grid_alert_all_set_phase(&grid_led_state, 100);
    }

    // Check if USB is connected and start animation
    if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && tud_connected()) {

      printf("USB CONNECTED\r\n\r\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 200);
      grid_msg_set_heartbeat_type(&grid_msg_state, 1);
    }

    // gpio_ll_set_level(&GPIO, 47, 1);

    handle_sync_ticks();

    handle_connect_disconnect_effect(partner_last_status);

    uint8_t port_list_length = grid_transport_get_port_array_length(&grid_transport_state);

    grid_midi_rx_pop(); // send_everywhere pushes to UI->RX_BUFFER

    struct grid_port* host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);
    struct grid_port* ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);

    struct grid_doublebuffer* host_doublebuffer_tx = grid_transport_get_doublebuffer_tx(host_port->parent, host_port->index);
    struct grid_doublebuffer* host_doublebuffer_rx = grid_transport_get_doublebuffer_rx(host_port->parent, host_port->index);

    {
      char message[GRID_PARAMETER_PACKET_maxlength + 100] = {0};
      uint16_t length = 0;

      if (host_doublebuffer_rx == NULL) {
        grid_platform_printf("NO USB RX DOUBLE BUFFER\n");
        abort();
      }

      grid_port_rxdobulebuffer_to_linear(host_port, host_doublebuffer_rx, message, &length); // USB

      grid_str_transform_brc_params(message, host_port->dx, host_port->dy, host_port->partner_fi); // update age, sx, sy, dx, dy, rot etc...
      grid_port_receive_decode(host_port, &recent_messages, message, length);
    }

    // INBOUND

    portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&spinlock);
    // Critical section to make sure that SPI interrupt cannot run while we are processing inbound messages
    // to protect UI_TX_BUFFER and HOST_TX_BUFFER
    // todo: move process_inbount for these buffers into SPI ISR
    for (uint8_t i = 0; i < port_list_length; i++) {

      struct grid_port* por = grid_transport_get_port(&grid_transport_state, i);
      struct grid_buffer* rx_buffer = grid_transport_get_buffer_rx(por->parent, por->index);

      grid_port_process_inbound(por, rx_buffer);
    }

    portEXIT_CRITICAL(&spinlock);

    // plot_port_debug();

    // OUTBOUND

    // NEED DELAY BETWEEN REQUESTING USB TRANSACTIONS (MIDI, HID, SERIAL etc)
    for (uint8_t i = 0; i < 4; i++) {

      grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);
      ets_delay_us(20);

      grid_midi_tx_pop();
      ets_delay_us(20);
    }

    struct grid_buffer* host_tx_buffer = grid_transport_get_buffer_tx(host_port->parent, host_port->index);
    grid_port_process_outbound_usb(host_port, host_tx_buffer, host_doublebuffer_tx); // WRITE TO USB SERIAL

    struct grid_buffer* ui_tx_buffer = grid_transport_get_buffer_tx(ui_port->parent, ui_port->index);
    grid_port_process_outbound_ui(ui_port, ui_tx_buffer);

    for (uint8_t i = 0; i < port_list_length; i++) {
      struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);
      struct grid_doublebuffer* doublebuffer_tx = grid_transport_get_doublebuffer_tx(&grid_transport_state, i);

      if (port->type == GRID_PORT_TYPE_USART) {
        struct grid_buffer* port_tx_buffer = grid_transport_get_buffer_tx(port->parent, port->index);
        grid_port_process_outbound_usart(port, port_tx_buffer, doublebuffer_tx);
      }
    }

    // gpio_ll_set_level(&GPIO, 47, 0);
    try_send_ping();
    try_send_heartbeat();

    // PORT PROCESS UI
    if (grid_lua_state.L != NULL && !grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {

      process_ui_local();
      process_ui_normal();
    }

    portYIELD();
  }

  ESP_LOGI(TAG, "Deinit PORT");
  vTaskSuspend(NULL);
}

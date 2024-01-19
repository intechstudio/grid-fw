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

#include "grid_port.h"
#include "grid_sys.h"

#include "driver/spi_slave.h"

extern uint32_t grid_platform_get_cycles(void);

static TaskHandle_t xTaskToNotify = NULL;

static const char* TAG = "PORT";

#define GPIO_MOSI 8
#define GPIO_MISO 6
#define GPIO_SCLK 9
#define GPIO_CS 7
#define RCV_HOST SPI2_HOST

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

  return;

  uint16_t length = strlen(str);

  // ets_printf("%s: ", tag);
  for (uint8_t i = 0; i < length; i++) {

    if (str[i] < 32) {

      // ets_printf("[%x] ", str[i]);
    } else {
      // ets_printf("%c ", str[i]);
    }
  }
  // ets_printf("\r\n");
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
  portEXIT_CRITICAL(&spinlock);
}

static void* DRAM_ATTR rx_debug = 0;
static uint8_t DRAM_ATTR rx_flag = 0;

static char DRAM_ATTR rx_str[500] = {0};

static struct grid_port* DRAM_ATTR uart_port_array[4] = {0};

static void IRAM_ATTR my_post_trans_cb(spi_slave_transaction_t* trans) {

  // ets_printf(" %d ", queue_state);
  rx_flag = 1;

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

  uint8_t ready_flags = ((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_STATUS_FLAGS_index];

  for (uint8_t i = 0; i < 4; i++) {
    struct grid_port* por = uart_port_array[i];

    if ((ready_flags & (0b00000001 << i))) {

      // por->tx_double_buffer_status = 0;

      if (por->tx_double_buffer_status == UINT16_MAX) {

        por->tx_double_buffer_status = 0;
      } else if (por->tx_double_buffer_status > 0) {

        por->tx_double_buffer_status = UINT16_MAX;
      }
    }
  }

  struct grid_port* por = NULL;
  uint8_t source_flags = ((uint8_t*)trans->rx_buffer)[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];

  if ((source_flags & 0b00000001)) {
    por = uart_port_array[0];
  }

  if ((source_flags & 0b00000010)) {
    por = uart_port_array[1];
  }

  if ((source_flags & 0b00000100)) {
    por = uart_port_array[2];
  }

  if ((source_flags & 0b00001000)) {
    por = uart_port_array[3];
  }

  if (por == NULL) {
    // no message to copy to rx_double_buffer
    return;
  }

  for (uint16_t i = 0; true; i++) {

    por->rx_double_buffer[por->rx_double_buffer_write_index] = ((char*)trans->rx_buffer)[i];

    if (((char*)trans->rx_buffer)[i] == '\0') {
      break;
    }

    por->rx_double_buffer_write_index++;
    por->rx_double_buffer_write_index %= GRID_DOUBLE_BUFFER_RX_SIZE;
  }
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

    struct grid_port* por = grid_transport_get_port(&grid_transport_state, i);

    plot[i + 0] = grid_buffer_get_space(&por->tx_buffer);
    plot[i + 6] = grid_buffer_get_space(&por->rx_buffer);
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

SemaphoreHandle_t nvm_or_port;

static uint64_t last_ping_timestamp = 0;
static uint64_t last_heartbeat_timestamp = 0;

void grid_esp32_port_periodic_ping_heartbeat_handler_cb(void* arg) {

  if (grid_platform_rtc_get_elapsed_time(last_ping_timestamp) < GRID_PARAMETER_PING_interval * 1000) {
    return;
  }

  if (xSemaphoreTake(nvm_or_port, portMAX_DELAY) == pdTRUE) {

    grid_protocol_send_heartbeat(); // Put heartbeat into UI rx_buffer
    last_heartbeat_timestamp = grid_platform_rtc_get_micros();

    if (uart_port_array[0] != NULL)
      uart_port_array[0]->ping_flag = 1;
    if (uart_port_array[1] != NULL)
      uart_port_array[1]->ping_flag = 1;
    if (uart_port_array[2] != NULL)
      uart_port_array[2]->ping_flag = 1;
    if (uart_port_array[3] != NULL)
      uart_port_array[3]->ping_flag = 1;

    grid_port_ping_try_everywhere();
    last_ping_timestamp = grid_platform_rtc_get_micros();

    xSemaphoreGive(nvm_or_port);
  }
}

void grid_esp32_port_task(void* arg) {

  nvm_or_port = (SemaphoreHandle_t)arg;

  uart_port_array[0] = grid_transport_get_port(&grid_transport_state, 0);
  uart_port_array[1] = grid_transport_get_port(&grid_transport_state, 1);
  uart_port_array[2] = grid_transport_get_port(&grid_transport_state, 2);
  uart_port_array[3] = grid_transport_get_port(&grid_transport_state, 3);

  uint8_t n = 0;
  esp_err_t ret;

  // Configuration for the SPI bus
  spi_bus_config_t buscfg = {.mosi_io_num = GPIO_MOSI, .miso_io_num = GPIO_MISO, .sclk_io_num = GPIO_SCLK, .quadwp_io_num = -1, .quadhd_io_num = -1, .intr_flags = ESP_INTR_FLAG_IRAM};

  // Configuration for the SPI slave interface
  spi_slave_interface_config_t slvcfg = {.mode = 0, .spics_io_num = GPIO_CS, .queue_size = 6, .flags = 0, .post_setup_cb = my_post_setup_cb, .post_trans_cb = my_post_trans_cb};

  // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master
  // is connected.
  gpio_set_pull_mode(GPIO_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);

  // Initialize SPI slave interface
  ret = spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
  assert(ret == ESP_OK);

  WORD_ALIGNED_ATTR char sendbuf[GRID_PARAMETER_SPI_TRANSACTION_length + 1] = {0};
  WORD_ALIGNED_ATTR char recvbuf[GRID_PARAMETER_SPI_TRANSACTION_length + 1] = {0};

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);

    // Set up a transaction of GRID_PARAMETER_SPI_TRANSACTION_length bytes to
    // send/receive

    memset(&outbnound_transaction[i], 0, sizeof(outbnound_transaction[i]));
    outbnound_transaction[i].length = GRID_PARAMETER_SPI_TRANSACTION_length * 8;
    outbnound_transaction[i].tx_buffer = port->tx_double_buffer;
    outbnound_transaction[i].rx_buffer = recvbuf;
  }

  empty_tx_buffer[0] = 'X';

  spi_empty_transaction.length = GRID_PARAMETER_SPI_TRANSACTION_length * 8;
  spi_empty_transaction.tx_buffer = empty_tx_buffer;
  spi_empty_transaction.rx_buffer = recvbuf;

  static uint32_t loopcounter = 0;

  static uint32_t cooldown = 0;

  SemaphoreHandle_t spi_ready_sem = xSemaphoreCreateBinary();

  portENTER_CRITICAL(&spinlock);
  ret = spi_slave_queue_trans(RCV_HOST, &spi_empty_transaction, 0);
  ESP_ERROR_CHECK(ret);
  if (ret == ESP_OK) {
    queue_state++;
  }
  portEXIT_CRITICAL(&spinlock);

  uint8_t firstprint = 1;

  // Create a periodic timer for thread safe miscellaneous tasks

  esp_timer_create_args_t periodic_ping_heartbeat_args = {.callback = &grid_esp32_port_periodic_ping_heartbeat_handler_cb, .name = "ping"};

  esp_timer_handle_t periodic_ping_heartbeat_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_ping_heartbeat_args, &periodic_ping_heartbeat_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_ping_heartbeat_timer, 10));

  // gpio_set_direction(47, GPIO_MODE_OUTPUT);

  while (1) {

    // Check if USB is connected and start animation
    if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && tud_connected()) {

      printf("USB CONNECTED\r\n\r\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 200);
      grid_msg_set_heartbeat_type(&grid_msg_state, 1);
    }

    // gpio_ll_set_level(&GPIO, 47, 1);

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

    if (xSemaphoreTake(nvm_or_port, pdMS_TO_TICKS(4)) == pdTRUE) {

      if (rx_flag != 0) {

        if (rx_debug != 0) {

          // ets_printf("\r\n%c> %s\r\n", grid_port_get_name_char((struct
          // grid_port*) rx_debug), rx_str);

          rx_debug = 0;
        } else {
          // ets_printf("#");
        }

        rx_flag = 0;
      }

      uint32_t c0, c1;

      uint8_t port_list_length = grid_transport_get_port_array_length(&grid_transport_state);
      // TRY TO RECEIVE UP TO 4 packets on UART PORTS
      for (uint8_t i = 0; i < port_list_length * 4; i++) {
        struct grid_port* port = grid_transport_get_port(&grid_transport_state, i % port_list_length);

        if (port->type == GRID_PORT_TYPE_USART) {
          grid_port_receive_task(port);
        }
      }

      loopcounter++;

      c0 = grid_platform_get_cycles();

      if (cooldown > 0) {
        cooldown--;
      }

      if (loopcounter % 2 == 0 && cooldown == 0) {

        // pending local events must be evaluated first
        // local events are only triggered when configuration changes!
        // WARNING: all init events are triggered as local after pagechange!
        if (grid_ui_event_count_istriggered_local(&grid_ui_state) && !grid_ui_bluk_anything_is_in_progress(&grid_ui_state)) {

          // CRITICAL_SECTION_ENTER()
          vTaskSuspendAll();
          grid_port_process_ui_local_UNSAFE(&grid_ui_state);
          xTaskResumeAll();
          // CRITICAL_SECTION_LEAVE()

        } else if (grid_ui_event_count_istriggered(&grid_ui_state) && !grid_ui_bluk_anything_is_in_progress(&grid_ui_state)) {

          cooldown += 3;

          // CRITICAL_SECTION_ENTER()
          vTaskSuspendAll();
          grid_port_process_ui_UNSAFE(&grid_ui_state);
          xTaskResumeAll();
          // CRITICAL_SECTION_LEAVE()
        }
      }

      c1 = grid_platform_get_cycles();

      grid_midi_rx_pop(); // send_everywhere pushes to UI->RX_BUFFER

      struct grid_port* host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);
      struct grid_port* ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);
      grid_port_receive_task(host_port); // USB
      grid_port_receive_task(ui_port);   // UI

      // INBOUND

      for (uint8_t i = 0; i < port_list_length; i++) {

        struct grid_port* por = grid_transport_get_port(&grid_transport_state, i);

        grid_port_process_inbound(por);
      }

      // plot_port_debug();

      // OUTBOUND

      // NEED DELAY BETWEEN REQUESTING USB TRANSACTIONS (MIDI, HID, SERIAL etc)
      for (uint8_t i = 0; i < 4; i++) {

        grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);
        ets_delay_us(20);

        grid_midi_tx_pop();
        ets_delay_us(20);
      }

      grid_port_process_outbound_usb(host_port); // WRITE TO USB SERIAL

      grid_port_process_outbound_ui(ui_port);

      for (uint8_t i = 0; i < port_list_length; i++) {
        struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);

        if (port->type == GRID_PORT_TYPE_USART) {
          grid_port_process_outbound_usart(port);
        }
      }

      uint32_t delta = c1 - c0;

      // grid_platform_printf("(%ld)us\r\n",
      // delta/grid_platform_get_cycles_per_us());

      xSemaphoreGive(nvm_or_port);
    } else {

      // ets_printf("NO TAKE\r\n");
      // NVM task is in progress, let it run!
      // vTaskDelay(pdMS_TO_TICKS(10));
    }

    // gpio_ll_set_level(&GPIO, 47, 0);

    portYIELD();
  }

  ESP_LOGI(TAG, "Deinit PORT");
  vTaskSuspend(NULL);
}

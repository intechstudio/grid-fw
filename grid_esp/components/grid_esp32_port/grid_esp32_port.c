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
#include "grid_esp32_lcd.h"
#include "grid_esp32_pins.h"
#include "grid_esp32_platform.h"
#include "grid_rollid.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_ui.h"
#include "grid_usb.h"
#include "grid_utask.h"

#include "driver/spi_slave.h"

static const char* TAG = "PORT";

#define RCV_HOST SPI2_HOST

volatile uint8_t DRAM_ATTR is_vsn_rev_a = 0;

spi_slave_transaction_t DRAM_ATTR spitra_empty = {0};
uint8_t DRAM_ATTR spitra_empty_tx_buf[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

spi_slave_transaction_t DRAM_ATTR spitra_usart[4] = {0};
uint8_t DRAM_ATTR spitra_usart_tx_buf[4][GRID_PARAMETER_SPI_TRANSACTION_length] = {0};
uint32_t DRAM_ATTR spitra_usart_tx_len[4] = {0};

// TODO figure out if this can lose the +1 of the terminating zero byte
uint8_t WORD_ALIGNED_ATTR spitra_rx_buf[GRID_PARAMETER_SPI_TRANSACTION_length + 1] = {0};

struct grid_rollid DRAM_ATTR rollid = {0};

void grid_platform_sync1_pulse_send() {}

portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR my_post_setup_cb(spi_slave_transaction_t* trans) {

  uint8_t* spi_tx_buf = (uint8_t*)trans->tx_buffer;

  uint8_t rollid_send = grid_rollid_send(&rollid);
  spi_tx_buf[GRID_PARAMETER_SPI_ROLLING_ID_index] = rollid_send;

  uint8_t pwm = grid_esp32_lcd_drawn ? grid_esp32_lcd_backlight : 0;
  pwm = is_vsn_rev_a ? 255 - pwm : pwm;
  spi_tx_buf[GRID_PARAMETER_SPI_BACKLIGHT_PWM_index] = pwm;
}

uint8_t IRAM_ATTR spitra_to_dir(spi_slave_transaction_t* trans) {

  uint8_t dir = (trans == &spitra_usart[0]) * 0 + (trans == &spitra_usart[1]) * 1 + (trans == &spitra_usart[2]) * 2 + (trans == &spitra_usart[3]) * 3;

  assert(dir < 4);

  return dir;
}

struct esp32_pico_spi_t {

  struct grid_swsr_t queue;
  uint8_t in_queue[4];
  uint8_t cooldown[4];
};

struct esp32_pico_spi_t DRAM_ATTR esp32_pico_spi;

void esp32_pico_spi_malloc(struct esp32_pico_spi_t* espico, int capacity) {

  assert(capacity > 0);

  assert(grid_swsr_malloc(&espico->queue, capacity * sizeof(void*)) == 0);

  memset(espico->in_queue, 0, 4 * sizeof(uint8_t));
  memset(espico->cooldown, 0, 4 * sizeof(uint8_t));
}

void IRAM_ATTR esp32_pico_spi_enqueue(struct esp32_pico_spi_t* espico, spi_slave_transaction_t* trans, uint8_t dir) {

  assert(dir < 4);

  assert(grid_swsr_writable(&espico->queue, sizeof(void*)));

  grid_swsr_write(&espico->queue, &trans, sizeof(void*));

  ++espico->in_queue[dir];
}

spi_slave_transaction_t* IRAM_ATTR esp32_pico_spi_dequeue(struct esp32_pico_spi_t* espico) {

  assert(grid_swsr_readable(&espico->queue, sizeof(void*)));

  spi_slave_transaction_t* trans;

  grid_swsr_read(&espico->queue, &trans, sizeof(void*));

  uint8_t dir = spitra_to_dir(trans);

  espico->in_queue[dir] -= espico->in_queue[dir] > 0;

  // Set a predefined cooldown for that port:
  // +1 to process the status_flags of the previous transaction,
  // +1 to process the status_flags of the current transaction,
  // after that point, the status_flags is valid for that direction
  espico->cooldown[dir] = 2;

  return trans;
}

uint8_t IRAM_ATTR esp32_pico_spi_dir_free(struct esp32_pico_spi_t* espico, uint8_t dir) {

  assert(dir < 4);

  return espico->in_queue[dir] == 0 && espico->cooldown[dir] == 0;
}

void IRAM_ATTR esp32_pico_spi_cooldown(struct esp32_pico_spi_t* espico) {

  for (int i = 0; i < 4; ++i) {

    espico->cooldown[i] -= espico->cooldown[i] > 0;
  }
}

static void IRAM_ATTR my_post_trans_cb(spi_slave_transaction_t* trans) {

  spi_slave_transaction_t* spitra = &spitra_empty;

  if (grid_swsr_readable(&esp32_pico_spi.queue, sizeof(void*))) {

    portENTER_CRITICAL(&spinlock);
    spitra = esp32_pico_spi_dequeue(&esp32_pico_spi);
    portEXIT_CRITICAL(&spinlock);
  }

  // Queue next SPI transaction
  spi_slave_queue_trans(RCV_HOST, spitra, 0);

  uint8_t* spi_rx_buf = (uint8_t*)trans->rx_buffer;

  grid_rollid_recv(&rollid, spi_rx_buf[GRID_PARAMETER_SPI_ROLLING_ID_index]);

  size_t length = strnlen((const char*)spi_rx_buf, GRID_PARAMETER_SPI_TRANSACTION_length);
  grid_transport_recv_usart(&grid_transport_state, spi_rx_buf, length);

  portENTER_CRITICAL(&spinlock);
  {
    uint8_t status_flags = spi_rx_buf[GRID_PARAMETER_SPI_STATUS_FLAGS_index];

    for (int i = 0; i < 4; ++i) {

      uint8_t flag = (status_flags >> i) & 0x1;

      if (esp32_pico_spi_dir_free(&esp32_pico_spi, i) && flag) {

        spitra_usart_tx_len[i] = 0;
      }
    }

    esp32_pico_spi_cooldown(&esp32_pico_spi);
  }
  portEXIT_CRITICAL(&spinlock);
}

struct grid_utask_timer timer_sendfull;

void grid_utask_sendfull(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_transport_sendfull(&grid_transport_state);
}

struct grid_utask_timer timer_ping;

void grid_utask_ping(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_transport_ping_all(&grid_transport_state);
}

struct grid_utask_timer timer_heart;

void grid_utask_heart(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  uint8_t type = grid_msg_get_heartbeat_type(&grid_msg_state);
  uint32_t hwcfg = grid_sys_get_hwcfg(&grid_sys_state);
  uint8_t activepage = grid_ui_state.page_activepage;
  grid_transport_heartbeat(&grid_transport_state, type, hwcfg, activepage);
}

struct grid_utask_timer timer_midi_and_keyboard_tx;

void grid_utask_midi_and_keyboard_tx(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_midi_tx_pop();

  ets_delay_us(20);

  grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);
}

struct grid_utask_timer timer_process_ui;

void grid_utask_process_ui(struct grid_utask_timer* timer) {

  if (grid_lua_state.L == NULL) {
    return;
  }

  if (grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {
    return;
  }

  // Service local triggers first and as fast as possible
  if (grid_ui_event_count_istriggered_local(&grid_ui_state) > 0) {

    grid_port_process_ui_local_UNSAFE(&grid_ui_state);
    return;
  }

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  if (grid_ui_event_count_istriggered(&grid_ui_state) > 0) {

    grid_port_process_ui_UNSAFE(&grid_ui_state);
  }
}

struct grid_utask_timer timer_midi_rx;

void grid_utask_midi_rx(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_midi_rx_pop();
}

// TODO direction should possibly remain in character-space, not [0, 4)
// or rewrite d51 as well.

// TODO thus, the dir param should be an enum

uint32_t grid_platform_get_frame_len(uint8_t dir) {

  assert(dir < GRID_PORT_DIR_COUNT);

  if (!grid_swsr_writable(&esp32_pico_spi.queue, sizeof(void*))) {
    return 1;
  }

  return spitra_usart_tx_len[dir];
}

void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir) {

  assert(swsr);
  assert(size > 0);
  assert(size <= GRID_PARAMETER_SPI_TRANSACTION_length - 1);
  assert(dir < GRID_PORT_DIR_COUNT);
  assert(grid_swsr_readable(swsr, size));
  assert(spitra_usart_tx_len[dir] == 0);
  assert(grid_swsr_writable(&esp32_pico_spi.queue, sizeof(void*)));

  spitra_usart_tx_len[dir] = size;

  spi_slave_transaction_t* spitra = &spitra_usart[dir];

  uint8_t* spi_tx_buf = (uint8_t*)spitra->tx_buffer;

  struct grid_swsr_t* tx = (struct grid_swsr_t*)swsr;

  grid_swsr_read(tx, spi_tx_buf, size);
  spi_tx_buf[size] = '\0';
  spi_tx_buf[GRID_PARAMETER_SPI_SOURCE_FLAGS_index] = 1 << dir;

  portENTER_CRITICAL(&spinlock);
  esp32_pico_spi_enqueue(&esp32_pico_spi, spitra, dir);
  portEXIT_CRITICAL(&spinlock);
}

void handle_connection_effect() {

  struct grid_transport* transport = &grid_transport_state;

  for (uint8_t i = 0; i < 4; ++i) {

    struct grid_port* port = grid_transport_get_port(transport, i, GRID_PORT_USART, i);

    if (!grid_port_connected_changed(port)) {
      continue;
    }

    if (grid_port_connected(port)) {

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
    }

    if (grid_port_disconnected(port)) {

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);

      grid_port_softreset(port);
    }

    grid_port_connected_update(port);
  }
}

bool grid_esp32_broadcast_between(enum grid_port_type t1, enum grid_port_type t2) { return !(t1 == GRID_PORT_USART && t2 == GRID_PORT_USART); }

void grid_esp32_port_task(void* arg) {

  // Set up "outbound usart" spi transactions
  size_t spitra_usart_bits = GRID_PARAMETER_SPI_TRANSACTION_length * 8;
  for (int i = 0; i < 4; ++i) {

    spitra_usart[i].length = spitra_usart_bits;
    spitra_usart[i].tx_buffer = spitra_usart_tx_buf[i];
    spitra_usart[i].rx_buffer = spitra_rx_buf;
  }

  // Set up empty transaction
  memset(&spitra_empty, 0, sizeof(spi_slave_transaction_t));
  spitra_empty.length = spitra_usart_bits;
  spitra_empty.tx_buffer = spitra_empty_tx_buf;
  spitra_empty.rx_buffer = spitra_rx_buf;

  // Initialize rolling ID
  grid_rollid_init(&rollid);

  // Configuration for the SPI bus
  spi_bus_config_t buscfg = {
      .mosi_io_num = GRID_ESP32_PINS_RP_MOSI,
      .miso_io_num = GRID_ESP32_PINS_RP_MISO,
      .sclk_io_num = GRID_ESP32_PINS_RP_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .intr_flags = ESP_INTR_FLAG_IRAM,
  };

  // Configuration for the SPI slave interface
  spi_slave_interface_config_t slvcfg = {
      .spics_io_num = GRID_ESP32_PINS_RP_CS,
      .queue_size = 1,
      .post_setup_cb = my_post_setup_cb,
      .post_trans_cb = my_post_trans_cb,
  };

  // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master
  // is connected.
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GRID_ESP32_PINS_RP_CS, GPIO_PULLUP_ONLY);

  // Initialize SPI slave interface
  ESP_ERROR_CHECK(spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));

  // Store whether this module is VSNX Rev A
  is_vsn_rev_a = grid_hwcfg_module_is_vsnx_rev_a(&grid_sys_state);

  // Watchdog-style tracking for the rolling ID
  uint8_t watchdog_rollid_last_recv = rollid.last_recv;
  uint64_t watchdog_rollid_last_time = grid_platform_rtc_get_micros();

  // Allocate custom SPI transaction queue
  esp32_pico_spi_malloc(&esp32_pico_spi, 6);

  // Queue an empty transaction initially
  // TODO detect if transactions stopped for any reason and restart
  spi_slave_queue_trans(RCV_HOST, &spitra_empty, 0);

  // Configure task timers
  timer_sendfull = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 1000000,
  };
  timer_ping = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = GRID_PARAMETER_PINGINTERVAL_us,
  };
  timer_heart = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = GRID_PARAMETER_HEARTBEATINTERVAL_us,
  };
  timer_midi_and_keyboard_tx = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 20,
  };
  timer_process_ui = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = GRID_PARAMETER_UICOOLDOWN_us,
  };
  timer_midi_rx = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 1000,
  };

  struct grid_transport* xport = &grid_transport_state;

  while (1) {

    // When the rolling ID changes, reset watchdog
    if (rollid.last_recv != watchdog_rollid_last_recv) {
      watchdog_rollid_last_time = grid_platform_rtc_get_micros();
      watchdog_rollid_last_recv = rollid.last_recv;
    }

    // Rolling ID watchdog expiration
    if (grid_platform_rtc_get_elapsed_time(watchdog_rollid_last_time) > 100000) {

      grid_platform_printf("ERROR: SPI frozen\n");

      watchdog_rollid_last_time = grid_platform_rtc_get_micros();

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 50);
      grid_alert_all_set_frequency(&grid_led_state, -4);
      grid_alert_all_set_phase(&grid_led_state, 100);
    }

    // Check if USB is connected and start animation
    if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && tud_connected()) {

      grid_platform_printf("USB CONNECTED\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 200);

      grid_msg_set_heartbeat_type(&grid_msg_state, 1);
    }

    // Editor timeout
    if (grid_sys_get_editor_connected_state(&grid_sys_state)) {

      uint32_t last = grid_msg_get_editor_heartbeat_lastrealtime(&grid_msg_state);
      if (grid_platform_rtc_get_elapsed_time(last) > 2000000) {

        grid_platform_printf("EDITOR TIMEOUT\n");

        grid_sys_set_editor_connected_state(&grid_sys_state, 0);
        // grid_ui_state.page_change_enabled = 1;
      }
    }

    struct grid_port* port_ui = grid_transport_get_port(xport, 4, GRID_PORT_UI, 0);
    struct grid_port* port_usb = grid_transport_get_port(xport, 5, GRID_PORT_USB, 0);

    // Broadcast inbound to outbound
    for (uint8_t i = 0; i < 4; ++i) {

      struct grid_port* port = grid_transport_get_port(xport, i, GRID_PORT_USART, i);

      grid_transport_rx_broadcast_tx(xport, port, grid_esp32_broadcast_between);
    }
    grid_transport_rx_broadcast_tx(xport, port_ui, grid_esp32_broadcast_between);
    grid_transport_rx_broadcast_tx(xport, port_usb, grid_esp32_broadcast_between);

    // Run microtasks
    grid_utask_sendfull(&timer_sendfull);
    grid_utask_ping(&timer_ping);
    grid_utask_heart(&timer_heart);
    grid_utask_midi_and_keyboard_tx(&timer_midi_and_keyboard_tx);
    grid_utask_process_ui(&timer_process_ui);
    grid_utask_midi_rx(&timer_midi_rx);

    // Outbound USB
    grid_port_send_usb(port_usb);

    // Outbound UI
    grid_port_send_ui(port_ui);

    // Outbound USART
    grid_transport_send_usart_cyclic_offset(xport);

    // ets_delay_us(100);

    handle_connection_effect();

    portYIELD();
  }

  ESP_LOGI(TAG, "Deinit PORT");
  vTaskSuspend(NULL);
}

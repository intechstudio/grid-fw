#include "grid_d51_module.h"

#include "atmel_start_pins.h"
#include <atmel_start.h>

#include <hal_qspi_dma.h>

#include <stdio.h>

#include <hpl_reset.h>
#include <string.h>

#include "grid_msg.h"
#include "grid_port.h"

#include "usb/class/midi/device/audiodf_midi.h"

static volatile struct grid_port* uart_port_array[4] = {0};
static volatile struct grid_port* host_port = NULL;
static volatile struct grid_port* ui_port = NULL;

volatile uint32_t loopcounter = 1;
volatile uint32_t loopcount = 0;

static volatile uint8_t sync1_received = 0;
static volatile uint8_t sync2_received = 0;

static volatile uint8_t sync1_state = 0;
static volatile uint8_t sync1_drive = 0;

void grid_platform_sync1_pulse_send() { sync1_state++; }

extern void grid_platform_rtc_set_micros(uint64_t mic);
extern uint64_t grid_platform_rtc_get_micros(void);

static void usb_task_inner(struct grid_msg_recent_buffer* rec) {

  grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);

  // Send midi from Grid to Host!

  for (uint8_t i = 0; i < 5; i++) {

    grid_midi_tx_pop();
  }

  // Forward midi from Host to Grid!
  grid_midi_rx_pop();

  // SERIAL READ

  char temp[GRID_PARAMETER_PACKET_maxlength + 100] = {0};
  uint16_t length = 0;

  struct grid_doublebuffer* doublebuffer_tx = grid_transport_get_doublebuffer_tx(&grid_transport_state, 5);
  struct grid_doublebuffer* doublebuffer_rx = grid_transport_get_doublebuffer_rx(&grid_transport_state, 5);

  grid_port_rxdobulebuffer_to_linear(host_port, doublebuffer_tx, doublebuffer_rx, temp, &length); // USB
  grid_port_receive_decode(host_port, rec, temp, length);
}

static void nvm_task_inner() {

  if (grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {
    grid_d51_nvic_set_interrupt_priority_mask(1);
  } else {
    if (grid_d51_nvic_get_interrupt_priority_mask() == 1) {
      // nvm just entered ready state

      // lets re-enable ui interrupts
      grid_d51_nvic_set_interrupt_priority_mask(0);
    }

    return;
  }

  uint64_t time_max_duration = 5 * 1000; // in microseconds
  uint64_t time_start = grid_platform_rtc_get_micros();

  do {

    switch (grid_ui_get_bulk_status(&grid_ui_state)) {
    case GRID_UI_BULK_READ_PROGRESS:
      grid_ui_bulk_pageread_next(&grid_ui_state);
      break;
    case GRID_UI_BULK_STORE_PROGRESS:
      grid_ui_bulk_pagestore_next(&grid_ui_state);
      break;
    case GRID_UI_BULK_CLEAR_PROGRESS:
      grid_ui_bulk_pageclear_next(&grid_ui_state);
      break;
    case GRID_UI_BULK_ERASE_PROGRESS:
      grid_ui_bulk_nvmerase_next(&grid_ui_state);
      break;
    default:
      break;
    }

  } while (grid_platform_rtc_get_elapsed_time(time_start) < time_max_duration && grid_ui_bulk_anything_is_in_progress(&grid_ui_state));
}

static void receive_task_inner(uint8_t* partner_connected, struct grid_msg_recent_buffer* rec) {

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);
    struct grid_doublebuffer* doublebuffer_tx = grid_transport_get_doublebuffer_tx(&grid_transport_state, i);
    struct grid_doublebuffer* doublebuffer_rx = grid_transport_get_doublebuffer_rx(&grid_transport_state, i);

    if (partner_connected[i] < port->partner_status) {
      // connect
      partner_connected[i] = 1;
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
    } else if (partner_connected[i] > port->partner_status) {

      // Disconnect
      partner_connected[i] = 0;
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
    }

    char temp[GRID_PARAMETER_PACKET_maxlength + 100] = {0};
    uint16_t length = 0;
    grid_port_rxdobulebuffer_to_linear(port, doublebuffer_tx, doublebuffer_rx, temp, &length);
    grid_port_receive_decode(port, rec, temp, length);
    grid_port_try_uart_timeout_disconect(port, doublebuffer_tx, doublebuffer_rx); // try disconnect for uart port
  }
}

static void ui_task_inner(uint8_t* ui_port_cooldown) {

  // every other entry of the superloop
  if (loopcount % 4 != 0) {
    return;
  }

  grid_port_ping_try_everywhere();

  // IF LOCAL MESSAGE IS AVAILABLE
  if (grid_ui_event_count_istriggered_local(&grid_ui_state)) {

    CRITICAL_SECTION_ENTER()
    grid_port_process_ui_local_UNSAFE(&grid_ui_state); // COOLDOWN DELAY IMPLEMENTED INSIDE
    CRITICAL_SECTION_LEAVE()
  }

  // Bandwidth Limiter for Broadcast messages

  if (*ui_port_cooldown > 0) {
    (*ui_port_cooldown)--;
  }

  if (*ui_port_cooldown > 5) {
    return;
  }

  // if there are still unprocessed locally triggered events then must not
  // serve global events yet!
  if (grid_ui_event_count_istriggered_local(&grid_ui_state)) {
    return;
  }

  if (grid_ui_event_count_istriggered(&grid_ui_state)) {

    *ui_port_cooldown += 3;

    CRITICAL_SECTION_ENTER()
    grid_port_process_ui_UNSAFE(&grid_ui_state);
    CRITICAL_SECTION_LEAVE()
  }
}

static void inbound_task_inner() {

  /* ========================= GRID INBOUND TASK =============================
   */

  // Copy data from UI_RX to HOST_TX & north TX AND STUFF

  grid_port_process_inbound(ui_port); // Loopback

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, i);
    grid_port_process_inbound(port);
  }

  grid_port_process_inbound(host_port); // USB
}

static void outbound_task_inner() {

  /* ========================= GRID OUTBOUND TASK =============================
   */

  // If previous xfer is completed and new data is available then move data from
  // txbuffer to txdoublebuffer and start new xfer.

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* port = uart_port_array[i];
    struct grid_doublebuffer* doublebuffer_tx = grid_transport_get_doublebuffer_tx(&grid_transport_state, i);

    grid_port_process_outbound_usart(port, doublebuffer_tx);
  }

  grid_port_process_outbound_ui(ui_port);

  struct grid_doublebuffer* host_doublebuffer_tx = grid_transport_get_doublebuffer_tx(&grid_transport_state, 5);
  grid_port_process_outbound_usb(host_port, host_doublebuffer_tx);
}

static uint64_t led_lastrealtime = 0;

static void led_task_inner() {

  if (10 * 1000 < grid_platform_rtc_get_elapsed_time(led_lastrealtime)) {

    led_lastrealtime = grid_platform_rtc_get_micros();

    grid_led_tick(&grid_led_state);

    grid_led_render_framebuffer(&grid_led_state);

    grid_d51_led_generate_frame(&grid_d51_led_state, &grid_led_state);

    grid_d51_led_start_transfer(&grid_d51_led_state);
  }
}

volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;
volatile uint8_t reportflag = 0;
volatile uint8_t heartbeatflag = 0;

static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_realtime_ms;
static struct timer_task RTC_Scheduler_grid_sync;
static struct timer_task RTC_Scheduler_heartbeat;
static struct timer_task RTC_Scheduler_report;

void RTC_Scheduler_ping_cb(const struct timer_task* const timer_task) {

  pingflag++;
  uart_port_array[pingflag % 4]->ping_flag = 1;
}

#define RTC1SEC 16384
#define RTC1MS (RTC1SEC / 1000)

void RTC_Scheduler_realtime_cb(const struct timer_task* const timer_task) {

  uint64_t micros = grid_platform_rtc_get_micros();
  micros += 1000000 / RTC1SEC; // 1 000 000 us / 16384TICK/SEC = 1 TICK
  grid_platform_rtc_set_micros(micros);
}

void RTC_Scheduler_realtime_millisecond_cb(const struct timer_task* const timer_task) {

  grid_ui_rtc_ms_tick_time(&grid_ui_state);
  grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, !gpio_get_pin_level(MAP_MODE));
}

void RTC_Scheduler_grid_sync_cb(const struct timer_task* const timer_task) {
  CRITICAL_SECTION_ENTER()

  while (sync1_received) {
    grid_ui_midi_sync_tick_time(&grid_ui_state);
    sync1_received--;
    // printf("s");
  }

  while (sync2_received) {
    grid_ui_midi_sync_tick_time(&grid_ui_state);
    sync2_received--;
    // printf("s");
  }

  // if sync 1 was driven by this module then trigger sync tick manually because
  // interrupts cannot trigger on gpio's that are set as output
  if (sync1_drive == 1) {
    grid_ui_midi_sync_tick_time(&grid_ui_state);
  }

  if (sync1_state) {

    sync1_state--;

    gpio_set_pin_pull_mode(PIN_GRID_SYNC_1, GPIO_PULL_DOWN);
    sync1_drive = 1;

    gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(PIN_GRID_SYNC_1, 1);
    // set_drive_mode(SYNC1_PIN, GPIO_DRIVE_MODE_OPEN_DRAIN);
  } else if (sync1_drive) {
    gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_IN);
    sync1_drive = 0;
  }

  CRITICAL_SECTION_LEAVE()
}

void RTC_Scheduler_heartbeat_cb(const struct timer_task* const timer_task) { heartbeatflag = 1; }

void RTC_Scheduler_report_cb(const struct timer_task* const timer_task) { reportflag = 1; }

void init_timer(void) {

  RTC_Scheduler_ping.interval = RTC1MS * GRID_PARAMETER_PING_interval;
  RTC_Scheduler_ping.cb = RTC_Scheduler_ping_cb;
  RTC_Scheduler_ping.mode = TIMER_TASK_REPEAT;

  RTC_Scheduler_heartbeat.interval = RTC1MS * GRID_PARAMETER_HEARTBEAT_interval;
  RTC_Scheduler_heartbeat.cb = RTC_Scheduler_heartbeat_cb;
  RTC_Scheduler_heartbeat.mode = TIMER_TASK_REPEAT;

  RTC_Scheduler_realtime.interval = 1;
  RTC_Scheduler_realtime.cb = RTC_Scheduler_realtime_cb;
  RTC_Scheduler_realtime.mode = TIMER_TASK_REPEAT;

  RTC_Scheduler_realtime_ms.interval = RTC1MS * 1;
  RTC_Scheduler_realtime_ms.cb = RTC_Scheduler_realtime_millisecond_cb;
  RTC_Scheduler_realtime_ms.mode = TIMER_TASK_REPEAT;

  RTC_Scheduler_grid_sync.interval = RTC1MS / 2;
  RTC_Scheduler_grid_sync.cb = RTC_Scheduler_grid_sync_cb;
  RTC_Scheduler_grid_sync.mode = TIMER_TASK_REPEAT;

  RTC_Scheduler_report.interval = RTC1MS * 1000;
  RTC_Scheduler_report.cb = RTC_Scheduler_report_cb;
  RTC_Scheduler_report.mode = TIMER_TASK_REPEAT;

  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_ping);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_heartbeat);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime_ms);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_grid_sync);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);

  timer_start(&RTC_Scheduler);
}

//====================== USB TEST =====================//

static void button_on_SYNC1_pressed(void) { sync1_received++; }

static void button_on_SYNC2_pressed(void) { sync2_received++; }

int main(void) {

  atmel_start_init(); // this sets up gpio and printf

  grid_platform_printf("Start Initialized %d %s\r\n", 123, "Cool!");

  grid_d51_init(); // Check User Row

  grid_sys_init(&grid_sys_state);
  grid_msg_init(&grid_msg_state);

  // grid_d51_nvm_erase_all(&grid_d51_nvm_state);

  printf("Hardware test complete");

  //  x/512xb 0x80000
  grid_module_common_init();

  uart_port_array[0] = grid_transport_get_port(&grid_transport_state, 0);
  uart_port_array[1] = grid_transport_get_port(&grid_transport_state, 1);
  uart_port_array[2] = grid_transport_get_port(&grid_transport_state, 2);
  uart_port_array[3] = grid_transport_get_port(&grid_transport_state, 3);

  ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);
  host_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_USB);

  grid_d51_usb_init(); // requires hostport

  grid_lua_init(&grid_lua_state);
  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb
  grid_lua_start_vm(&grid_lua_state);
  grid_lua_ui_init(&grid_lua_state, &grid_ui_state);

  grid_d51_led_init(&grid_d51_led_state, &grid_led_state);

  printf("Start TOC init\r\n");
  grid_d51_nvm_toc_init(&grid_d51_nvm_state);
  // grid_d51_nvm_toc_debug(&grid_d51_nvm_state);
  printf("Done TOC init\r\n");
  grid_ui_page_load(&grid_ui_state, 0); // load page 0

  while (grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {
    grid_ui_bulk_pageread_next(&grid_ui_state);
  }

  // grid_d51_nvm_toc_debug(&grid_d51_nvm_state);

  init_timer();

  uint32_t loopstart = 0;

#ifdef GRID_BUILD_UNKNOWN
  printf("\r\n##Build: Unknown##\r\n\r\n");
#endif
#ifdef GRID_BUILD_NIGHTLY
  printf("\r\n##Build: Nightly##\r\n\r\n");
#endif
#ifdef GRID_BUILD_DEBUG
  printf("\r\n##Build: Debug##\r\n\r\n");
#endif
#ifdef GRID_BUILD_RELEASE
  printf("\r\n##Build: Release##\r\n\r\n");
#endif

  ext_irq_register(PIN_GRID_SYNC_1, button_on_SYNC1_pressed);
  ext_irq_register(PIN_GRID_SYNC_2, button_on_SYNC2_pressed);

  // partner_connected array holds the last state. This is used for checking changes and triggering led effects accordingly
  uint8_t partner_connected[grid_transport_get_port_array_length(&grid_transport_state)];
  memset(partner_connected, 0, grid_transport_get_port_array_length(&grid_transport_state));

  struct grid_msg_recent_buffer recent_messages;
  grid_msg_recent_fingerprint_buffer_init(&recent_messages, 32);

  uint8_t ui_port_cooldown = 0;

  while (1) {

    if (usb_d_get_frame_num() != 0) {

      if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1) {

        printf("USB CONNECTED\r\n\r\n");
        printf("HWCFG %d\r\n", grid_sys_get_hwcfg(&grid_sys_state));

        grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
        grid_alert_all_set_frequency(&grid_led_state, -2);
        grid_alert_all_set_phase(&grid_led_state, 200);

        grid_msg_set_heartbeat_type(&grid_msg_state, 1);

        printf("Register MIDI callbacks\r\n\r\n");
        // grid_d51_usb_midi_register_callbacks();
      }
    }

    // printf("WTF\r\n\r\n");

    loopcounter++;
    loopcount++;

    if (reportflag) {

      reportflag = 0;
      loopcount = 0;
    }

    if (loopcounter == 1000) {

      grid_ui_state.ui_interaction_enabled = 1;

      grid_d51_nvic_debug_priorities();
    }

    usb_task_inner(&recent_messages);

    nvm_task_inner();

    receive_task_inner(partner_connected, &recent_messages);

    // lua_gc(grid_lua_state.L, LUA_GCSTOP);

    ui_task_inner(&ui_port_cooldown);

    outbound_task_inner();

    inbound_task_inner();

    led_task_inner();

    if (heartbeatflag) {

      heartbeatflag = 0;

      grid_protocol_send_heartbeat();
    }

    if (grid_sys_get_editor_connected_state(&grid_sys_state) == 1) {

      if (grid_platform_rtc_get_elapsed_time(grid_msg_get_editor_heartbeat_lastrealtime(&grid_msg_state)) > 2000 * MS_TO_US) { // 2 sec

        printf("EDITOR timeout\r\n");
        grid_port_debug_print_text("EDITOR timeout");

        grid_sys_set_editor_connected_state(&grid_sys_state, 0);

        grid_ui_state.page_change_enabled = 1;
      }
    }

  } // WHILE

} // MAIN

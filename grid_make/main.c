#include "grid_d51_module.h"

#include "grid_d51_led.h"
#include "grid_d51_nvm.h"
#include "grid_d51_uart.h"

#include "atmel_start_pins.h"
#include <atmel_start.h>

#include <hal_qspi_dma.h>

#include <stdio.h>

#include <hpl_reset.h>
#include <string.h>

#include "grid_msg.h"
#include "grid_port.h"
#include "grid_utask.h"

#include "vmp_def.h"
#include "vmp_tag.h"

#include "usb/class/midi/device/audiodf_midi.h"

extern const struct luaL_Reg* grid_lua_api_generic_lib_reference;

#include <stdio.h>
#include <stdlib.h>

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

  grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);

  for (uint8_t i = 0; i < 5; i++) {

    grid_midi_tx_pop();
  }
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

struct grid_utask_timer timer_led;

void grid_utask_led(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_led_tick(&grid_led_state);

  grid_led_render_framebuffer(&grid_led_state);

  grid_d51_led_generate_frame(&grid_d51_led_state, &grid_led_state);

  grid_d51_led_start_transfer(&grid_d51_led_state);
}

volatile uint8_t rxtimeoutselector = 0;

volatile uint8_t pingflag = 0;
volatile uint8_t reportflag = 0;

static struct timer_task RTC_Scheduler_rx_task;
static struct timer_task RTC_Scheduler_ping;
static struct timer_task RTC_Scheduler_realtime;
static struct timer_task RTC_Scheduler_realtime_ms;
static struct timer_task RTC_Scheduler_grid_sync;
static struct timer_task RTC_Scheduler_heartbeat;
static struct timer_task RTC_Scheduler_report;

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

void RTC_Scheduler_report_cb(const struct timer_task* const timer_task) { reportflag = 1; }

void init_timer(void) {

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

  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_realtime_ms);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_grid_sync);
  timer_add_task(&RTC_Scheduler, &RTC_Scheduler_report);

  timer_start(&RTC_Scheduler);
}

//====================== USB TEST =====================//

static void button_on_SYNC1_pressed(void) { sync1_received++; }

static void button_on_SYNC2_pressed(void) { sync2_received++; }

int grid_uwsr_cspn_terminated(struct grid_uwsr_t* uwsr) {

  int first = uwsr->read + 1;
  int j = (first + uwsr->seek) % uwsr->capacity;

  int read = uwsr->read;
  while (j != read && uwsr->data[j] != uwsr->reject && uwsr->data[j]) {
    j = (first + (++uwsr->seek)) % uwsr->capacity;
  }

  if (uwsr->seek < 3) {
    return -1;
  }

  int eot_idx = (j - 3 + uwsr->capacity) % uwsr->capacity;
  bool eot_before_checksum = uwsr->data[eot_idx] == GRID_CONST_EOT;

  bool found = j != read && uwsr->data[j] == uwsr->reject && eot_before_checksum;

  return found ? uwsr->seek : -1;
}

void grid_d51_port_recv_uwsr(struct grid_port* port, struct grid_uwsr_t* uwsr, struct grid_msg_recent_buffer* recent) {

  if (grid_uwsr_overflow(uwsr)) {

    grid_uwsr_init(uwsr, uwsr->reject);

    grid_platform_reset_grid_transmitter(grid_port_dir_to_code(port->dir));
  }

  int ret = grid_uwsr_cspn_terminated(uwsr);

  if (ret < 0) {
    return;
  }

  if (ret >= GRID_PARAMETER_SPI_TRANSACTION_length) {

    grid_uwsr_read(uwsr, NULL, ret + 1);

    return;
  }

  uint8_t temp[GRID_PARAMETER_SPI_TRANSACTION_length + 1];

  grid_uwsr_read(uwsr, temp, ret + 1);

  temp[ret + 1] = '\0';

  if (grid_str_verify_frame(temp, ret + 1) != 0) {
    return;
  }

  grid_str_transform_brc_params(temp, port->dx, port->dy, port->partner.rot);

  uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(temp);

  if (temp[1] == GRID_CONST_BRC) {

    if (grid_msg_recent_fingerprint_find(recent, fingerprint)) {
      return;
    }

    grid_msg_recent_fingerprint_store(recent, fingerprint);
  }

  grid_port_recv_msg(port, temp, ret + 1);
}

int main(void) {

  atmel_start_init(); // this sets up gpio and printf

  grid_platform_printf("Start Initialized %d %s\r\n", 123, "Cool!");

  grid_d51_init(); // Check User Row

  grid_sys_init(&grid_sys_state);
  grid_msg_init(&grid_msg_state);

  // grid_d51_nvm_erase_all(&grid_d51_nvm_state);

  printf("Hardware test complete\n");

  //  x/512xb 0x80000
  grid_module_common_init();

  grid_d51_usb_init(); // requires hostport

  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb
  grid_lua_start_vm(&grid_lua_state);
  grid_lua_vm_register_functions(&grid_lua_state, grid_lua_api_generic_lib_reference);
  grid_lua_ui_init(&grid_lua_state, grid_ui_state.lua_ui_init_callback);

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
#ifdef GRID_BUILD_DEBUG
  printf("\r\n##Build: Debug##\r\n\r\n");
#endif
#ifdef GRID_BUILD_RELEASE
  printf("\r\n##Build: Release##\r\n\r\n");
#endif

  ext_irq_register(PIN_GRID_SYNC_1, button_on_SYNC1_pressed);
  ext_irq_register(PIN_GRID_SYNC_2, button_on_SYNC2_pressed);

  struct grid_msg_recent_buffer recent;
  grid_msg_recent_fingerprint_buffer_init(&recent, 64);

  // Configure task timers
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
  timer_led = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 10000,
  };
  timer_process_ui = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = GRID_PARAMETER_UICOOLDOWN_us,
  };

  struct grid_transport* xport = &grid_transport_state;

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };
  bool vmp_flushed = false;

  while (1) {

    // vmp_push(MAIN);

    if (!vmp_flushed && vmp.size == vmp.capacity) {

      CRITICAL_SECTION_ENTER();

      vmp_serialize_start(&reg);
      vmp_buf_serialize_and_write(&vmp, &reg);
      vmp_uid_str_serialize_and_write(VMP_UID_COUNT, VMP_ASSOC, &reg);
      vmp_serialize_close(&reg);

      CRITICAL_SECTION_LEAVE();

      // vmp_buf_free(&vmp);

      vmp_flushed = true;
    }

    loopcounter++;

    if (loopcounter == 10000) {

      // grid_d51_nvic_debug_priorities();
    }

    // Check if USB is connected and start animation
    if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && usb_d_get_frame_num()) {

      grid_platform_printf("USB CONNECTED\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 200);

      grid_msg_set_heartbeat_type(&grid_msg_state, 1);
    }

    grid_midi_rx_pop();

    // NVM task
    nvm_task_inner();

    // Receive USART
    for (uint8_t i = 0; i < 4; ++i) {

      struct grid_port* port = grid_transport_get_port(xport, i, GRID_PORT_USART, i);
      struct grid_uwsr_t* uwsr = &usart_uwsr[i];

      grid_d51_port_recv_uwsr(port, uwsr, &recent);
    }

    struct grid_port* port_ui = grid_transport_get_port(xport, 4, GRID_PORT_UI, 0);
    struct grid_port* port_usb = grid_transport_get_port(xport, 5, GRID_PORT_USB, 0);

    // Broadcast inbound to outbound
    for (uint8_t i = 0; i < 4; ++i) {

      struct grid_port* port = grid_transport_get_port(xport, i, GRID_PORT_USART, i);

      grid_transport_rx_broadcast_tx(xport, port, NULL);
    }
    grid_transport_rx_broadcast_tx(xport, port_ui, NULL);
    grid_transport_rx_broadcast_tx(xport, port_usb, NULL);

    // Run microtasks
    grid_utask_ping(&timer_ping);
    grid_utask_heart(&timer_heart);
    grid_utask_midi_and_keyboard_tx(&timer_midi_and_keyboard_tx);
    grid_utask_led(&timer_led);
    grid_utask_process_ui(&timer_process_ui);

    // Outbound USB
    grid_port_send_usb(port_usb);

    // Outbound UI
    grid_port_send_ui(port_ui);

    // Outbound USART
    grid_transport_send_usart_cyclic_offset(xport);

    handle_connection_effect();

  } // WHILE

} // MAIN

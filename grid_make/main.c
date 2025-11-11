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

#include "grid_allocator.h"
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

void grid_platform_lcd_set_backlight(uint8_t backlight) {}

static void update_interrupt_mask_from_bulk_status() {

  uint32_t mask = grid_d51_nvic_get_interrupt_priority_mask() == 1;
  uint32_t next = grid_ui_bulk_anything_is_in_progress(&grid_ui_state);

  if (mask != next) {

    grid_d51_nvic_set_interrupt_priority_mask(next);
  }
}

static void nvm_task_inner() {

  update_interrupt_mask_from_bulk_status();

  uint64_t time_max_duration = 10 * 1000; // in microseconds
  uint64_t time_start = grid_platform_rtc_get_micros();

  bool proceed = grid_ui_bulk_anything_is_in_progress(&grid_ui_state);
  while (proceed) {

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
    case GRID_UI_BULK_CONFREAD_PROGRESS:
      grid_ui_bulk_confread_next(&grid_ui_state);
      break;
    case GRID_UI_BULK_CONFSTORE_PROGRESS:
      grid_ui_bulk_confstore_next(&grid_ui_state);
      break;
    default:
      break;
    }

    proceed = grid_platform_rtc_get_elapsed_time(time_start) < time_max_duration;
    proceed = proceed && grid_ui_bulk_anything_is_in_progress(&grid_ui_state);
  }
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
  grid_lua_semaphore_lock(&grid_lua_state);
  uint8_t gccount = grid_lua_gc_count_unsafe(&grid_lua_state);
  grid_lua_semaphore_release(&grid_lua_state);
  grid_transport_heartbeat(&grid_transport_state, type, hwcfg, activepage, gccount);
}

struct grid_utask_timer timer_midi_and_keyboard_tx;

void grid_utask_midi_and_keyboard_tx(struct grid_utask_timer* timer) {

  if (grid_midi_tx_readable()) {

    if (!grid_utask_timer_elapsed(timer)) {
      return;
    }

    for (uint8_t i = 0; i < 4; ++i) {
      grid_midi_tx_pop();
    }
  }

  if (grid_usb_keyboard_tx_readable(&grid_usb_keyboard_state)) {

    if (!grid_utask_timer_elapsed(timer)) {
      return;
    }

    grid_usb_keyboard_tx_pop(&grid_usb_keyboard_state);
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

  if (grid_ui_event_count_istriggered(&grid_ui_state) > 0) {

    if (!grid_utask_timer_elapsed(timer)) {
      return;
    }

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

struct grid_utask_timer timer_midi_rx;

void grid_utask_midi_rx(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_midi_rx_pop();
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

void grid_d51_port_recv_uwsr(struct grid_port* port, struct grid_uwsr_t* uwsr, struct grid_fingerprint_buf* fpb) {

  if (grid_uwsr_overflow(uwsr)) {

    grid_uwsr_init(uwsr, uwsr->reject);

    grid_platform_reset_grid_transmitter(grid_port_dir_to_code(port->dir));
  }

  struct grid_msg msg;

  if (!grid_msg_from_uwsr(&msg, uwsr)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) != 0) {
    return;
  }

  grid_str_transform_brc_params((uint8_t*)msg.data, msg.length, port->dx, port->dy, port->partner.rot);

  uint32_t fingerprint = grid_fingerprint_calculate(msg.data);

  if (msg.data[1] == GRID_CONST_BRC) {

    if (grid_fingerprint_buf_find(fpb, fingerprint)) {
      return;
    }

    grid_fingerprint_buf_store(fpb, fingerprint);
  }

  grid_port_recv_msg(port, (uint8_t*)msg.data, msg.length);
}

int main(void) {

  atmel_start_init(); // this sets up gpio and printf

  grid_platform_printf("Start Initialized %d %s\r\n", 123, "Cool!");

  grid_d51_init(); // Check User Row

  grid_sys_init(&grid_sys_state);
  grid_msg_model_init(&grid_msg_state);

  // grid_d51_nvm_erase_all(&grid_d51_nvm_state);

  printf("Hardware test complete\n");

  //  x/512xb 0x80000
  grid_module_common_init();

  grid_d51_usb_init(); // requires hostport

  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb

  grid_d51_led_init(&grid_d51_led_state, &grid_led_state);

  grid_d51_nvic_debug_priorities();

  grid_ui_page_load(&grid_ui_state, 0); // load page 0

  while (grid_ui_bulk_anything_is_in_progress(&grid_ui_state)) {
    nvm_task_inner();
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

  struct grid_fingerprint_buf recent;
  grid_fingerprint_buf_init(&recent, 64);

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
  timer_led = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 10000,
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

    /*
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
    */

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

    // Editor timeout
    if (grid_sys_get_editor_connected_state(&grid_sys_state)) {

      uint64_t last = grid_msg_get_editor_heartbeat_lastrealtime(&grid_msg_state);
      if (grid_platform_rtc_get_elapsed_time(last) > 2000000) {

        grid_sys_set_editor_connected_state(&grid_sys_state, 0);
        grid_platform_printf("EDITOR TIMEOUT\n");
        // grid_ui_state.page_change_enabled = 1;
      }
    }

    grid_d51_midi_bulkout_poll();

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

    // Run receiver-type microtasks
    grid_utask_led(&timer_led);
    grid_utask_sendfull(&timer_sendfull);
    grid_utask_ping(&timer_ping);
    grid_utask_heart(&timer_heart);
    grid_utask_midi_rx(&timer_midi_rx);
    grid_utask_process_ui(&timer_process_ui);

    // Decode for USB
    grid_port_send_usb(port_usb);

    // Run transmitter-type microtasks
    grid_utask_midi_and_keyboard_tx(&timer_midi_and_keyboard_tx);

    // Decode for UI
    grid_port_send_ui(port_ui);

    // Outbound USART
    grid_transport_send_usart_cyclic_offset(xport);

    // Garbage collection step for lua
    grid_lua_semaphore_lock(&grid_lua_state);
    grid_lua_gc_step_unsafe(&grid_lua_state);
    grid_lua_semaphore_release(&grid_lua_state);

    handle_connection_effect();

  } // WHILE

} // MAIN

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "hardware/dma.h"
#include "hardware/irq.h"

#include "grid_ain.h"
#include "grid_health.h"
#include "grid_led.h"
#include "grid_lua.h"
#include "grid_module.h"
#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_port.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_ui.h"
#include "grid_usb.h"
#include "grid_utask.h"

#include "grid_rp2350_adc.h"
#include "grid_rp2350_led.h"
#include "grid_rp2350_module_bu16.h"
#include "grid_rp2350_nvm.h"
#include "grid_rp2350_usb.h"

// RP2350 has no LCD/GUI element type yet, so the Lua GUI library is empty,
// matching D51 (d51n20a/grid_d51n20a.c:23-24).
const struct luaL_Reg gui_lib[] = {{NULL, NULL}};
const struct luaL_Reg* grid_lua_api_gui_lib_reference = gui_lib;

enum { GRID_RP2350_PORT_COUNT = 2 };

// Mirrors ESP32's log_checkpoint (esp32s3/main/grid_esp32s3.c) so a boot hang
// shows exactly which init step it stalled after.
static void grid_rp2350_checkpoint(const char* label) { printf("[checkpoint] %s\n", label); }

// Bulk processing mutates ui->element_list non-atomically, racing the ADC IRQ
// (bu16_process_analog); D51 masks NVIC BASEPRI around bulk ops for the same
// reason (grid_d51n20a.c's update_interrupt_mask_from_bulk_status). RP2350
// only needs to gate DMA_IRQ_0 -- the LED driver's DMA transfer is polled, not
// interrupt-driven, so it's exclusively the ADC's IRQ.
static void grid_rp2350_update_interrupt_mask_from_bulk_status(void) { irq_set_enabled(DMA_IRQ_0, !grid_ui_bulk_in_progress(&grid_ui_state)); }

// Boot-count survives resets iff the littlefs block device round-trips.
static void grid_fs_bringup(void) {
  grid_rp2350_nvm_mount(&grid_rp2350_nvm_state, false);
  if (!grid_platform_get_nvm_state()) {
    printf("littlefs: mount failed\n");
    return;
  }

  uint32_t boot_count = 0;
  char* contents = grid_platform_read_file_contents("/bootcount");
  if (contents) {
    boot_count = (uint32_t)strtoul(contents, NULL, 10);
    free(contents);
  }

  boot_count++;

  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)boot_count);
  grid_platform_write_file_contents(buf, "/bootcount");

  printf("littlefs: boot count = %lu\n", (unsigned long)boot_count);
}

// Replaces D51's RTC_Scheduler_realtime_ms task: RP2350 already has a real
// hardware microsecond clock, so only the UI ms-tick hook itself is needed,
// driven by a hardware alarm instead of D51's software RTC emulation.
static struct repeating_timer grid_rp2350_ms_timer;

static bool grid_rp2350_ms_tick_cb(struct repeating_timer* t) {
  grid_ui_rtc_ms_tick_time(&grid_ui_state);
  return true;
}

static struct grid_utask_timer timer_led;

static void grid_utask_led(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_led_tick(&grid_led_state);
  grid_led_render_framebuffer(&grid_led_state);
  grid_rp2350_led_generate_frame(&grid_rp2350_led_state, &grid_led_state);
  grid_rp2350_led_start_transfer(&grid_rp2350_led_state);
}

static struct grid_utask_timer timer_heart;

static void grid_utask_heart(struct grid_utask_timer* timer) {

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

static struct grid_utask_timer timer_health_report;

static void grid_utask_health_report(void) {

  if (grid_usb_connected() && grid_usb_acm_dtr(&grid_usb_state.acm) && grid_utask_timer_elapsed(&timer_health_report)) {

    grid_health_report(&grid_health_state);
  }
}

static struct grid_utask_timer timer_process_ui;

static void grid_utask_process_ui(struct grid_utask_timer* timer) {

  if (grid_lua_state.L == NULL) {
    return;
  }

  if (grid_ui_events_any(&grid_ui_state)) {

    if (!grid_utask_timer_elapsed(timer)) {
      return;
    }

    grid_ui_process_triggered(&grid_ui_state);
  }
}

static struct grid_utask_timer timer_midi_rx;

static void grid_utask_midi_rx(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_usb_midi_rx_voice_process(&grid_usb_state.midi);
  grid_usb_midi_rx_sysex_process(&grid_usb_state.midi);
  grid_usb_midi_rx_rtm_process(&grid_usb_state.midi);
}

int main() {
  stdio_init_all();
  // stdio above is UART0 TX-only (GPIO0) + RTT (see rp2350/main/CMakeLists.txt
  // -- UART0 RX/GPIO1 was freed for the HWCFG SHIFT strap) -- USB CDC is the
  // framed grid_usb ACM channel below, not a printf console.
  printf("grid rp2350 bringup\n");

  grid_rp2350_usb_init();
  grid_rp2350_checkpoint("USB INIT");

  grid_sys_init(&grid_sys_state);
  printf("HWCFG: %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  grid_rp2350_checkpoint("SYS INIT");

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
    grid_rp2350_checkpoint("UI INIT (bu16)");
  } else {
    printf("UI Init failed: Unknown Module %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  }

  // Requires grid_led_state already populated by the UI-init dispatch above.
  grid_rp2350_led_init(&grid_rp2350_led_state, &grid_led_state);
  grid_rp2350_checkpoint("LED INIT");

  grid_fs_bringup();
  grid_rp2350_checkpoint("NVM MOUNT");

  grid_msg_model_init(&grid_msg_state);
  grid_rp2350_checkpoint("MSG INIT");

  grid_transport_malloc(&grid_transport_state, GRID_RP2350_PORT_COUNT);
  grid_port_init(&grid_transport_state.ports[0], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[1], GRID_PORT_USB, 0);
  grid_rp2350_checkpoint("TRANSPORT/PORT INIT");

  // Without this, active-bank color stays at its zero-init default (black),
  // so an "auto" (-1) element LED color -- which derives from it -- resolves
  // to black regardless of button state. D51/ESP32 both call this too.
  grid_sys_set_bank(&grid_sys_state, 0);
  grid_rp2350_checkpoint("BANK INIT");

  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_rp2350_checkpoint("LUA INIT");

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_rp2350_module_bu16_init(&grid_sys_state, &grid_ui_state, &grid_rp2350_adc_state, &grid_config_state, &grid_cal_state);
    grid_rp2350_checkpoint("MODULE INIT (bu16)");
  } else {
    printf("Module Init failed: Unknown Module %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  }

  add_repeating_timer_ms(-1, grid_rp2350_ms_tick_cb, NULL, &grid_rp2350_ms_timer);
  grid_rp2350_checkpoint("MS TICK TIMER ARMED");

  uint64_t now = grid_platform_rtc_get_micros();
  timer_led = (struct grid_utask_timer){.last = now, .period = 10000};
  timer_heart = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_HEARTBEATINTERVAL_us};
  timer_health_report = (struct grid_utask_timer){.last = now, .period = 1000000};
  timer_process_ui = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_UICOOLDOWN_us};
  timer_midi_rx = (struct grid_utask_timer){.last = now, .period = 1000};
  grid_rp2350_checkpoint("UTASK TIMERS SEEDED");

  grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_page_load, 0, 0, NULL);
  grid_rp2350_update_interrupt_mask_from_bulk_status();
  grid_ui_bulk_flush(&grid_ui_state);
  grid_rp2350_update_interrupt_mask_from_bulk_status();
  grid_rp2350_checkpoint("PAGE 0 BULK LOAD");

  struct grid_port* port_ui = grid_transport_get_port(&grid_transport_state, 0, GRID_PORT_UI, 0);
  struct grid_port* port_usb = grid_transport_get_port(&grid_transport_state, 1, GRID_PORT_USB, 0);

  grid_rp2350_checkpoint("ENTERING MAIN LOOP");

  while (true) {

    grid_usb_task();

    if (grid_msg_get_heartbeat_type(&grid_msg_state) != 1 && grid_usb_connected()) {

      printf("USB CONNECTED\n");

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 100);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 200);

      grid_msg_set_heartbeat_type(&grid_msg_state, 1);
    }

    if (grid_sys_get_editor_connected_state(&grid_sys_state)) {

      uint64_t last = grid_msg_get_editor_heartbeat_lastrealtime(&grid_msg_state);
      if (grid_platform_rtc_get_elapsed_time(last) > 2000000) {

        grid_sys_set_editor_connected_state(&grid_sys_state, 0);
        printf("EDITOR TIMEOUT\n");
      }
    }

    grid_usb_midi_rx_poll(&grid_usb_state.midi);
    grid_usb_acm_rx_poll(&grid_usb_state.acm);
    grid_usb_acm_rx_process(&grid_usb_state.acm);

    grid_rp2350_update_interrupt_mask_from_bulk_status();
    grid_ui_bulk_process(&grid_ui_state);

    grid_transport_rx_broadcast_tx(&grid_transport_state, port_ui, NULL);
    grid_transport_rx_broadcast_tx(&grid_transport_state, port_usb, NULL);

    grid_utask_led(&timer_led);
    grid_utask_heart(&timer_heart);
    grid_utask_midi_rx(&timer_midi_rx);
    grid_utask_process_ui(&timer_process_ui);

    grid_port_send_usb(port_usb);

    if (grid_usb_midi_tx_available(&grid_usb_state.midi)) {
      grid_usb_midi_tx_flush(&grid_usb_state.midi);
    }
    if (grid_usb_macro_tx_available(&grid_usb_state.hid.macro)) {
      grid_usb_macro_tx_flush(&grid_usb_state.hid.macro);
    }
    if (grid_usb_gamepad_tx_available(&grid_usb_state.hid.gamepad)) {
      grid_usb_gamepad_tx_flush(&grid_usb_state.hid.gamepad);
    }

    grid_utask_health_report();

    grid_port_send_ui(port_ui);

    grid_lua_semaphore_lock(&grid_lua_state);
    grid_lua_gc_step_unsafe(&grid_lua_state);
    grid_lua_semaphore_release(&grid_lua_state);
  }
}

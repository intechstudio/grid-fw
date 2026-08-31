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
#include "grid_rp2350_encoder.h"
#include "grid_rp2350_led.h"
#include "grid_rp2350_module_bu16.h"
#include "grid_rp2350_module_ef44.h"
#include "grid_rp2350_nvm.h"
#include "grid_rp2350_uart.h"
#include "grid_rp2350_usb.h"

// RP2350 has no LCD/GUI element type yet, so the Lua GUI library is empty,
// matching D51 (d51n20a/grid_d51n20a.c:23-24).
const struct luaL_Reg gui_lib[] = {{NULL, NULL}};
const struct luaL_Reg* grid_lua_api_gui_lib_reference = gui_lib;

// USART x4 (N/E/S/W), UI, USB -- the same fixed 6-port layout D51/ESP32 use
// (grid_transport.h's GRID_TRANSPORT_PORT_INDEX_UI/USB default to 4/5), now
// that grid_rp2350_uart.c provides the daisy-chain directions.
enum { GRID_RP2350_PORT_COUNT = 6 };

// GPIO6's mux entry for UART1 TX (see grid_rp2350_led.h for why the function
// number isn't the generic GPIO_FUNC_UART here). BU16 is a temporary
// exception on the current prototype hardware, using GPIO4 instead -- once
// the newer fixed BU16 prototype lands, it moves to GPIO6/F11 like every
// other variant and this whole per-variant pin selection goes away.
#define GRID_RP2350_GPIO6_FUNC_UART1_TX 11

// MAP_MODE button: external pull-up, active-low (pressed = 0). No internal
// pull needed since the pull-up is already on the board.
#define GRID_RP2350_MAPMODE_PIN 16

// Mirrors ESP32's log_checkpoint (esp32s3/main/grid_esp32s3.c) so a boot hang
// shows exactly which init step it stalled after.
static void grid_rp2350_checkpoint(const char* label) { printf("[checkpoint] %s\n", label); }

// Bulk processing mutates ui->element_list non-atomically, racing the ADC IRQ
// (bu16_process_analog) and, on EF44, the encoder IRQ (ef44_process_encoder)
// too; D51 masks NVIC BASEPRI around bulk ops for the same reason
// (grid_d51n20a.c's update_interrupt_mask_from_bulk_status). RP2350 gates
// DMA_IRQ_0 (ADC) and DMA_IRQ_1 (encoder, grid_rp2350_encoder.c) -- the LED
// driver's DMA transfer is polled, not interrupt-driven, so these two are the
// only DMA IRQ sources touching UI state.
static void grid_rp2350_update_interrupt_mask_from_bulk_status(void) {
  bool enabled = !grid_ui_bulk_in_progress(&grid_ui_state);
  irq_set_enabled(DMA_IRQ_0, enabled);
  irq_set_enabled(DMA_IRQ_1, enabled);
}

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
// driven by a hardware alarm instead of D51's software RTC emulation. Also
// drives the MAP_MODE button (D51/ESP32 poll it from their own 1ms tick the
// same way) -- grid_ui_rtc_ms_mapmode_handler is a plain common/src/c edge
// detector with no debounce, so this matches existing behavior exactly.
static struct repeating_timer grid_rp2350_ms_timer;

static bool grid_rp2350_ms_tick_cb(struct repeating_timer* t) {
  grid_ui_rtc_ms_tick_time(&grid_ui_state);
  grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, !gpio_get(GRID_RP2350_MAPMODE_PIN));
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

static struct grid_utask_timer timer_ping;

static void grid_utask_ping(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  grid_transport_ping_all(&grid_transport_state);
}

// Flashes green/red on USART neighbor connect/disconnect and resets the
// disconnected direction's transmitter -- ports d51n20a/grid_d51n20a.c's
// handle_connection_effect verbatim.
static void handle_connection_effect(void) {

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

    struct grid_port* port = grid_transport_get_port(&grid_transport_state, dir, GRID_PORT_USART, dir);

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

  gpio_init(GRID_RP2350_MAPMODE_PIN);
  gpio_set_dir(GRID_RP2350_MAPMODE_PIN, GPIO_IN);

  grid_rp2350_usb_init();
  grid_rp2350_checkpoint("USB INIT");

  grid_sys_init(&grid_sys_state);
  printf("HWCFG: %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  grid_rp2350_checkpoint("SYS INIT");

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
    grid_rp2350_checkpoint("UI INIT (bu16)");
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
    grid_rp2350_checkpoint("UI INIT (ef44)");
  } else {
    printf("UI Init failed: Unknown Module %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  }

  // BU16's current prototype hardware still uses GPIO4/GPIO_FUNC_UART instead
  // of GPIO6/F11 like every other variant (GPIO4 is EF44's SPI0 MISO, so the
  // two can't share a pin) -- temporary until the newer fixed BU16 prototype
  // moves it to GPIO6/F11 too, at which point this ternary goes away.
  // Requires grid_led_state already populated by the UI-init dispatch above.
  bool is_bu16 = grid_hwcfg_module_is_bu16(&grid_sys_state);
  uint8_t led_tx_pin = is_bu16 ? 4 : 6;
  uint8_t led_tx_pin_func = is_bu16 ? GPIO_FUNC_UART : GRID_RP2350_GPIO6_FUNC_UART1_TX;
  grid_rp2350_led_init(&grid_rp2350_led_state, &grid_led_state, led_tx_pin, led_tx_pin_func);
  grid_rp2350_checkpoint("LED INIT");

  grid_fs_bringup();
  grid_rp2350_checkpoint("NVM MOUNT");

  grid_msg_model_init(&grid_msg_state);
  grid_rp2350_checkpoint("MSG INIT");

  grid_transport_malloc(&grid_transport_state, GRID_RP2350_PORT_COUNT);
  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {
    grid_port_init(&grid_transport_state.ports[dir], GRID_PORT_USART, dir);
  }
  grid_port_init(&grid_transport_state.ports[GRID_TRANSPORT_PORT_INDEX_UI], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[GRID_TRANSPORT_PORT_INDEX_USB], GRID_PORT_USB, 0);
  grid_rp2350_uart_init();
  grid_rp2350_checkpoint("TRANSPORT/PORT/UART INIT");

  // Without this, active-bank color stays at its zero-init default (black),
  // so an "auto" (-1) element LED color -- which derives from it -- resolves
  // to black regardless of button state. D51/ESP32 both call this too.
  grid_sys_set_bank(&grid_sys_state, 0);
  grid_rp2350_checkpoint("BANK INIT");

  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_rp2350_checkpoint("LUA INIT");

  add_repeating_timer_ms(-1, grid_rp2350_ms_tick_cb, NULL, &grid_rp2350_ms_timer);
  grid_rp2350_checkpoint("MS TICK TIMER ARMED");

  uint64_t now = grid_platform_rtc_get_micros();
  timer_led = (struct grid_utask_timer){.last = now, .period = 10000};
  timer_ping = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_PINGINTERVAL_us};
  timer_heart = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_HEARTBEATINTERVAL_us};
  timer_health_report = (struct grid_utask_timer){.last = now, .period = 1000000};
  timer_process_ui = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_UICOOLDOWN_us};
  timer_midi_rx = (struct grid_utask_timer){.last = now, .period = 1000};
  grid_rp2350_checkpoint("UTASK TIMERS SEEDED");

  // Page 0 must be loaded (ui->element_list's template_parameter_list
  // allocated/populated) before module init below starts the ADC/encoder --
  // their ISRs read that array on literally the first sample, and hardware
  // can complete a conversion in microseconds, faster than any interrupt
  // mask toggled after the fact could reliably win the race. ESP32 orders
  // its own bring-up the same way (page load before module init); D51
  // instead starts its ADC first and masks interrupts around the page load,
  // which turns out not to fully close this window (see project memory).
  grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_page_load, 0, 0, NULL);
  grid_ui_bulk_flush(&grid_ui_state);
  grid_rp2350_checkpoint("PAGE 0 BULK LOAD");

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_rp2350_module_bu16_init(&grid_sys_state, &grid_ui_state, &grid_rp2350_adc_state, &grid_config_state, &grid_cal_state);
    grid_rp2350_checkpoint("MODULE INIT (bu16)");
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_rp2350_module_ef44_init(&grid_sys_state, &grid_ui_state, &grid_rp2350_adc_state, &grid_rp2350_encoder_state, &grid_config_state, &grid_cal_state);
    grid_rp2350_checkpoint("MODULE INIT (ef44)");
  } else {
    printf("Module Init failed: Unknown Module %lu\n", (unsigned long)grid_sys_get_hwcfg(&grid_sys_state));
  }

  struct grid_port* port_ui = grid_transport_get_port(&grid_transport_state, GRID_TRANSPORT_PORT_INDEX_UI, GRID_PORT_UI, 0);
  struct grid_port* port_usb = grid_transport_get_port(&grid_transport_state, GRID_TRANSPORT_PORT_INDEX_USB, GRID_PORT_USB, 0);

  // De-dupes broadcast messages relayed across the 4 daisy-chain directions;
  // mirrors d51n20a/grid_d51n20a.c's `recent`.
  struct grid_fingerprint_buf recent;
  grid_fingerprint_buf_init(&recent, 64);

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

    for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

      struct grid_port* port = grid_transport_get_port(&grid_transport_state, dir, GRID_PORT_USART, dir);

      grid_rp2350_uart_port_recv(port, &grid_rp2350_uart_uwsr[dir], &recent);
    }

    for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

      struct grid_port* port = grid_transport_get_port(&grid_transport_state, dir, GRID_PORT_USART, dir);

      grid_transport_rx_broadcast_tx(&grid_transport_state, port, NULL);
    }
    grid_transport_rx_broadcast_tx(&grid_transport_state, port_ui, NULL);
    grid_transport_rx_broadcast_tx(&grid_transport_state, port_usb, NULL);

    grid_utask_led(&timer_led);
    grid_utask_ping(&timer_ping);
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

    grid_transport_send_usart_cyclic_offset(&grid_transport_state);

    grid_lua_semaphore_lock(&grid_lua_state);
    grid_lua_gc_step_unsafe(&grid_lua_state);
    grid_lua_semaphore_release(&grid_lua_state);

    handle_connection_effect();
  }
}

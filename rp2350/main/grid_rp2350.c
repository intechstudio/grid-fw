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

const struct luaL_Reg gui_lib[] = {{NULL, NULL}};
const struct luaL_Reg* grid_lua_api_gui_lib_reference = gui_lib;

enum { GRID_RP2350_PORT_COUNT = 6 };

// TODO BU16 should also use GPIO6, like the other variants do
#define GRID_RP2350_GPIO6_FUNC_UART1_TX 11

#define GRID_RP2350_MAPMODE_PIN 16

static void grid_rp2350_checkpoint(const char* label) { printf("[checkpoint] %s\n", label); }

static void grid_rp2350_update_interrupt_mask_from_bulk_status(void) {
  bool enabled = !grid_ui_bulk_in_progress(&grid_ui_state);
  irq_set_enabled(DMA_IRQ_0, enabled);
  irq_set_enabled(DMA_IRQ_1, enabled);
}

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

  gpio_init(GRID_RP2350_MAPMODE_PIN);
  gpio_set_dir(GRID_RP2350_MAPMODE_PIN, GPIO_IN);

  grid_rp2350_checkpoint("USB INIT");
  grid_rp2350_usb_init();

  grid_rp2350_checkpoint("SYS INIT");
  grid_sys_init(&grid_sys_state);

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else {
    printf("UI Init failed: Unknown Module %u\n", grid_sys_get_hwcfg(&grid_sys_state));
  }

  // TODO BU16 should also use GPIO6, like the other variants do
  bool is_bu16 = grid_hwcfg_module_is_bu16(&grid_sys_state);
  uint8_t led_tx_pin = is_bu16 ? 4 : 6;
  uint8_t led_tx_pin_func = is_bu16 ? GPIO_FUNC_UART : GRID_RP2350_GPIO6_FUNC_UART1_TX;

  grid_rp2350_checkpoint("LED INIT");
  grid_rp2350_led_init(&grid_rp2350_led_state, &grid_led_state, led_tx_pin, led_tx_pin_func);

  grid_rp2350_checkpoint("NVM MOUNT");
  grid_rp2350_nvm_mount(&grid_rp2350_nvm_state, false);

  grid_rp2350_checkpoint("MSG INIT");
  grid_msg_model_init(&grid_msg_state);

  grid_rp2350_checkpoint("LUA INIT");
  grid_lua_init(&grid_lua_state, NULL, NULL);

  grid_transport_malloc(&grid_transport_state, GRID_RP2350_PORT_COUNT);
  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {
    grid_port_init(&grid_transport_state.ports[dir], GRID_PORT_USART, dir);
  }
  grid_port_init(&grid_transport_state.ports[GRID_TRANSPORT_UI], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[GRID_TRANSPORT_USB], GRID_PORT_USB, 0);

  grid_rp2350_uart_init();

  grid_rp2350_checkpoint("BANK INIT");
  grid_sys_set_bank(&grid_sys_state, 0);

  grid_rp2350_checkpoint("LOAD PAGE ZERO");
  grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_page_load, 0, 0, NULL);
  grid_ui_bulk_flush(&grid_ui_state);

  uint64_t now = grid_platform_rtc_get_micros();
  timer_led = (struct grid_utask_timer){.last = now, .period = 10000};
  timer_ping = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_PINGINTERVAL_us};
  timer_heart = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_HEARTBEATINTERVAL_us};
  timer_health_report = (struct grid_utask_timer){.last = now, .period = 1000000};
  timer_process_ui = (struct grid_utask_timer){.last = now, .period = GRID_PARAMETER_UICOOLDOWN_us};
  timer_midi_rx = (struct grid_utask_timer){.last = now, .period = 1000};

  if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_rp2350_module_bu16_init(&grid_sys_state, &grid_ui_state, &grid_rp2350_adc_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_rp2350_module_ef44_init(&grid_sys_state, &grid_ui_state, &grid_rp2350_adc_state, &grid_rp2350_encoder_state, &grid_config_state, &grid_cal_state);
  } else {
    printf("Module Init failed: Unknown Module %u\n", grid_sys_get_hwcfg(&grid_sys_state));
  }

  add_repeating_timer_ms(-1, grid_rp2350_ms_tick_cb, NULL, &grid_rp2350_ms_timer);

  struct grid_fingerprint_buf recent;
  grid_fingerprint_buf_init(&recent, 64);

  struct grid_transport* xport = &grid_transport_state;

  grid_rp2350_checkpoint("MAIN LOOP");

  while (true) {

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

    grid_rp2350_update_interrupt_mask_from_bulk_status();
    grid_ui_bulk_process(&grid_ui_state);

    for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

      struct grid_port* port = grid_transport_get_port(xport, dir, GRID_PORT_USART, dir);

      grid_rp2350_uart_port_recv(port, &grid_rp2350_uart_uwsr[dir], &recent);
    }

    struct grid_port* port_ui = grid_transport_get_port(xport, GRID_TRANSPORT_UI, GRID_PORT_UI, 0);
    struct grid_port* port_usb = grid_transport_get_port(xport, GRID_TRANSPORT_USB, GRID_PORT_USB, 0);

    for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

      struct grid_port* port = grid_transport_get_port(xport, dir, GRID_PORT_USART, dir);

      grid_transport_rx_broadcast_tx(xport, port, NULL);
    }
    grid_transport_rx_broadcast_tx(xport, port_ui, NULL);
    grid_transport_rx_broadcast_tx(xport, port_usb, NULL);

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

    grid_usb_task();

    grid_usb_midi_rx_poll(&grid_usb_state.midi);
    grid_usb_acm_rx_poll(&grid_usb_state.acm);
    grid_usb_acm_rx_process(&grid_usb_state.acm);

    grid_port_send_ui(port_ui);

    grid_transport_send_usart_cyclic_offset(xport);

    grid_lua_semaphore_lock(&grid_lua_state);
    grid_lua_gc_step_unsafe(&grid_lua_state);
    grid_lua_semaphore_release(&grid_lua_state);

    handle_connection_effect();
  }
}

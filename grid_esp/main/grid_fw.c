/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "usb/usb_host.h"

#include <string.h>

#include "driver/spi_master.h"
#include "rom/ets_sys.h" // For ets_printf

#include "driver/gpio.h"
#include "esp_flash.h"
#include "esp_task_wdt.h"

#include "grid_esp32_led.h"
#include "grid_esp32_module_bu16.h"
#include "grid_esp32_module_ef44.h"
#include "grid_esp32_module_en16.h"
#include "grid_esp32_module_pb44.h"
#include "grid_esp32_module_pbf4.h"
#include "grid_esp32_module_po16.h"
#include "grid_esp32_module_tek2.h"
#include "pico_firmware.h"

#include "grid_esp32_trace.h"

// module task priority must be the lowest to ma it run most of the time
#define MODULE_TASK_PRIORITY 0

#define LED_TASK_PRIORITY 2

// NVM must not be preemted by Port task
#define NVM_TASK_PRIORITY configMAX_PRIORITIES - 1

// module task priority must be the lowest to ma it run most of the time
#define PORT_TASK_PRIORITY 0 // same as idle

#include "driver/ledc.h"
#include <esp_timer.h>

#include "grid_esp32.h"
#include "grid_esp32_nvm.h"
#include "grid_esp32_port.h"
#include "grid_esp32_swd.h"
#include "grid_esp32_usb.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "rom/ets_sys.h" // For ets_printf

#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_buf.h"
#include "../../grid_common/grid_led.h"
#include "../../grid_common/grid_module.h"
#include "../../grid_common/grid_msg.h"
#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_protocol.h"
#include "../../grid_common/grid_sys.h"
#include "../../grid_common/grid_usb.h"

#include "../../grid_common/grid_lua_api.h"
#include "../../grid_common/grid_ui.h"

#include "../../grid_common/lua-5.4.3/src/lauxlib.h"
#include "../../grid_common/lua-5.4.3/src/lua.h"
#include "../../grid_common/lua-5.4.3/src/lualib.h"

static const char* TAG = "main";

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static void periodic_rtc_ms_cb(void* arg) {

  grid_ui_rtc_ms_tick_time(&grid_ui_state);

  if (gpio_get_level(GRID_ESP32_PINS_MAPMODE)) {
    grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 0);
  } else {
    grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 1);
  }
}

void system_init_core_2_task(void* arg) {

  grid_esp32_swd_pico_pins_init(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, GRID_ESP32_PINS_RP_CLOCK);
  grid_esp32_swd_pico_clock_init(LEDC_TIMER_0, LEDC_CHANNEL_0);
  grid_esp32_swd_pico_program_sram(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, pico_firmware, pico_firmware_len);

  vTaskSuspend(NULL);
}

bool idle_hook(void) {

  portYIELD();
  return 0;
}

static void check_heap(void) {

  uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  ESP_LOGI(TAG, "free RAM is %d. Integrity: %d", freeRAM, heap_caps_check_integrity_all(true));
}

void app_main(void) {

  // set console baud rate
  ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_0, 2000000ul));

  esp_log_level_set("*", ESP_LOG_INFO);

  SemaphoreHandle_t nvm_or_port = xSemaphoreCreateBinary();
  xSemaphoreGive(nvm_or_port);

  ESP_LOGI(TAG, "===== MAIN START =====");

  gpio_set_direction(GRID_ESP32_PINS_MAPMODE, GPIO_MODE_INPUT);
  gpio_pullup_en(GRID_ESP32_PINS_MAPMODE);

  ESP_LOGI(TAG, "===== SYS START =====");
  grid_sys_init(&grid_sys_state);

  ESP_LOGI(TAG, "===== UI INIT =====");
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevD) {
    grid_module_po16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevD) {
    grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevD) {
    grid_module_pbf4_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD) {
    grid_module_en16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD) {
    grid_module_en16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD) {
    grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK2_RevA) {
    grid_module_tek2_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PB44_RevA) {
    grid_module_pb44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else {
    ets_printf("Init Module: Unknown Module\r\n");
  }

  uint8_t led_pin = 21;

  grid_led_set_pin(&grid_led_state, led_pin);

  TaskHandle_t led_task_hdl;
  xTaskCreatePinnedToCore(grid_esp32_led_task, "led", 1024 * 3, NULL, LED_TASK_PRIORITY, &led_task_hdl, 0);

  TaskHandle_t usb_task_hdl;
  xTaskCreatePinnedToCore(grid_esp32_usb_task, "TinyUSB", 4096, NULL, 6, &usb_task_hdl, 1);

  TaskHandle_t core2_task_hdl;
  xTaskCreatePinnedToCore(system_init_core_2_task, "swd_init", 1024 * 3, NULL, 4, &core2_task_hdl, 1);

  // GRID MODULE INITIALIZATION SEQUENCE

  ESP_LOGI(TAG, "===== NVM START =====");
  xSemaphoreTake(nvm_or_port, 0);
  grid_esp32_nvm_init(&grid_esp32_nvm_state);

  if (gpio_get_level(GRID_ESP32_PINS_MAPMODE) == 0) {

    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, 1000);
    grid_alert_all_set_frequency(&grid_led_state, 4);
    grid_esp32_nvm_erase(&grid_esp32_nvm_state);
    vTaskDelay(pdMS_TO_TICKS(600));
  }

  xSemaphoreGive(nvm_or_port);

  ESP_LOGI(TAG, "===== MSG START =====");
  grid_msg_init(&grid_msg_state); // setup session id, last message buffer init

  ESP_LOGI(TAG, "===== LUA INIT =====");
  grid_lua_init(&grid_lua_state);
  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb

  grid_lua_start_vm(&grid_lua_state);
  grid_lua_ui_init(&grid_lua_state, &grid_ui_state);

  // ================== START: grid_module_pbf4_init() ================== //

  ESP_LOGI(TAG, "===== PORT INIT =====");

  grid_transport_init(&grid_transport_state);

  ESP_LOGI(TAG, "Transport done");

  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_NORTH, 0));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_EAST, 0));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_SOUTH, 0));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_WEST, 0));

  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_UI, 0, 1));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USB, 0, 0));

  ESP_LOGI(TAG, "Port done");

  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));

  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));

  ESP_LOGI(TAG, "Doublebuffer done");

  ESP_LOGI(TAG, "===== BANK INIT =====");
  grid_sys_set_bank(&grid_sys_state, 0);
  ets_delay_us(2000);

  grid_ui_page_load(&grid_ui_state, 0); // load page 0

  // while (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state))
  // {
  //     grid_ui_bulk_pageread_next(&grid_ui_state);
  // }

  SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  ESP_LOGI(TAG, "===== UI TASK INIT =====");

  TaskHandle_t module_task_hdl;
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_po16_task, "po16", 1024 * 4, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_bu16_task, "bu16", 1024 * 3, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_pbf4_task, "pbf4", 1024 * 3, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024 * 4, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024 * 4, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_ef44_task, "ef44", 1024 * 4, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK2_RevA) {
    xTaskCreatePinnedToCore(grid_esp32_module_tek2_task, "tek2", 1024 * 4, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PB44_RevA) {
    xTaskCreatePinnedToCore(grid_esp32_module_pb44_task, "pb44", 1024 * 3, (void*)nvm_or_port, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else {
    printf("Init Module: Unknown Module\r\n");
  }

  ESP_LOGI(TAG, "===== UI TASK DONE =====");

  grid_ui_state.ui_interaction_enabled = 1;
  // ================== FINISH: grid_module_pbf4_init() ================== //

  // Create the class driver task

  TaskHandle_t nvm_task_hdl;

  TaskHandle_t port_task_hdl;

  // Create the class driver task
  xTaskCreatePinnedToCore(grid_esp32_port_task, "port", 4096 * 10, (void*)nvm_or_port, PORT_TASK_PRIORITY, &port_task_hdl, 1);

  ESP_LOGI(TAG, "===== PORT TASK DONE =====");

  // Create the class driver task

  xTaskCreatePinnedToCore(grid_esp32_nvm_task, "nvm", 1024 * 10, (void*)nvm_or_port, NVM_TASK_PRIORITY, &nvm_task_hdl, 0);

  TaskHandle_t housekeeping_task_hdl;

  ESP_LOGI(TAG, "===== NVM TASK DONE =====");

  // Create the class driver task
  xTaskCreatePinnedToCore(grid_esp32_housekeeping_task, "housekeeping", 1024 * 6, (void*)signaling_sem, 6, &housekeeping_task_hdl, 0);

  TaskHandle_t grid_trace_report_task_hdl;

  ESP_LOGI(TAG, "===== HOUSE TASK DONE =====");

  xTaskCreatePinnedToCore(grid_trace_report_task, "trace", 1024 * 4, (void*)signaling_sem, 6, &grid_trace_report_task_hdl, 1);

  ESP_LOGI(TAG, "===== REPORT TASK DONE =====");

  esp_timer_create_args_t periodic_rtc_ms_args = {.callback = &periodic_rtc_ms_cb, .name = "rtc millisecond"};

  esp_timer_handle_t periodic_rtc_ms_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_rtc_ms_args, &periodic_rtc_ms_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_rtc_ms_timer, 1000));

  grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_WHITE_DIM, 100);

  ESP_LOGI(TAG, "===== INIT COMPLETE =====");

  // Register idle hook to force yield from idle task to lowest priority task
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 0);
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 1);

  ESP_LOGI(TAG, "===== TRACE START =====");

  // TRACE CONFIG GPIO 40, 41, 4Mbaud,

  // esp_apptrace_init();

  // SEGGER_SYSVIEW_RecordSystime();
  // SEGGER_SYSVIEW_SendTaskList();
  // SEGGER_SYSVIEW_SendNumModules();

  // SEGGER_SYSVIEW_Start();

  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
  gpio_pullup_en(GPIO_NUM_0);

  while (1) {

    // esp_sysview_flush(ESP_APPTRACE_TMO_INFINITE);

    if (gpio_get_level(GPIO_NUM_0) == 0) {

      // SEGGER_SYSVIEW_Stop();

      // esp_sysview_flush(ESP_APPTRACE_TMO_INFINITE);
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  ESP_LOGI(TAG, "===== MAIN COMPLETE =====");
}

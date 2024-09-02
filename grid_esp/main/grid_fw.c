/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_endless.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

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
#include "grid_esp32_module_tek1.h"
#include "grid_esp32_module_tek2.h"
#include "pico_firmware.h"

#include "grid_esp32_trace.h"

// module task priority must be the lowest to make it run most of the time
#define MODULE_TASK_PRIORITY 0

#define LED_TASK_PRIORITY 2

// NVM must not be preemted by Port task
#define NVM_TASK_PRIORITY configMAX_PRIORITIES - 1

// module task priority must be the lowest to ma it run most of the time
#define PORT_TASK_PRIORITY 0 // same as idle

#include "driver/ledc.h"
#include <esp_timer.h>

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "grid_esp32.h"
#include "grid_esp32_lcd.h"
#include "grid_esp32_nvm.h"
#include "grid_esp32_port.h"
#include "grid_esp32_swd.h"
#include "grid_esp32_usb.h"
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

struct grid_port grid_port_array[6] = {0};

struct grid_buffer grid_buffer_tx_array[6] = {0};
struct grid_buffer grid_buffer_rx_array[6] = {0};

char grid_buffer_tx_memory_array[6][GRID_BUFFER_SIZE] = {0};
char grid_buffer_rx_memory_array[6][GRID_BUFFER_SIZE] = {0};

struct grid_doublebuffer grid_doublebuffer_tx_array[6] = {0};
struct grid_doublebuffer grid_doublebuffer_rx_array[6] = {0};

char grid_doublebuffer_tx_memory_array[5][GRID_DOUBLE_BUFFER_TX_SIZE] = {0};
char grid_doublebuffer_rx_memory_array[2][GRID_DOUBLE_BUFFER_RX_SIZE] = {0};

#include "grid_lua_api_gui.h"
#include "grid_ui_lcd.h"

void grid_lua_ui_init_tek1(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_vm_register_functions(lua, grid_lua_api_gui_lib_reference);

  grid_lua_dostring(lua, GRID_LUA_B_META_init);
  grid_lua_dostring(lua, GRID_LUA_EP_META_init);
  grid_lua_dostring(lua, GRID_LUA_L_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 buttons
  grid_lua_dostring(lua, "for i=0, 7 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  // initialize 1 endless potentiometer
  grid_lua_dostring(lua, "for i=8, 8  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 8  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], endless_meta)  end");

  grid_lua_dostring(lua, "for i=9, 12 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=9, 12 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_dostring(lua, "for i=13, 13  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=13, 13  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], endless_meta)  end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[14] = {index = 14}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[14], system_meta)");
}

void grid_lua_ui_init_vsn2(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_vm_register_functions(lua, grid_lua_api_gui_lib_reference);

  grid_lua_dostring(lua, GRID_LUA_B_META_init);
  grid_lua_dostring(lua, GRID_LUA_L_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 buttons
  grid_lua_dostring(lua, "for i=0, 7 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_dostring(lua, "for i=8, 11 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 11 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_dostring(lua, "for i=12, 12 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=12, 12 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], lcd_meta) end");

  grid_lua_dostring(lua, "for i=13, 16 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=13, 16 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_dostring(lua, "for i=17, 17 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=17, 17 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], lcd_meta) end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[18] = {index = 18}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[18], system_meta)");
}

void grid_module_tek1_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui, uint8_t hwcfg) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);  // TODO: 12 ain for TEK2
  grid_led_init(led, 13 + 5); // TODO: 18 led for TEK2

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA) {

    uint8_t led_lookup[18] = {5, 6, 7, 8, 9, 10, 11, 12, 0, 17, 1, 17, 2, 17, 3, 17, 4, 17}; // set unused to -1
    grid_led_lookup_init(led, led_lookup);

  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA) {

    uint8_t led_lookup[18] = {10, 11, 12, 13, 14, 15, 16, 17, 5, 0, 6, 1, 7, 2, 8, 3, 9, 4}; // set unused to -1
    grid_led_lookup_init(led, led_lookup);

  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSNR_RevA) {

    uint8_t led_lookup[18] = {10, 11, 12, 13, 14, 15, 16, 17, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9}; // set unused to -1
    grid_led_lookup_init(led, led_lookup);

  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA) {

    grid_platform_printf("VSN2\n");
    uint8_t led_lookup[18] = {10, 11, 12, 13, 14, 15, 16, 17, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9}; // set unused to -1
    grid_led_lookup_init(led, led_lookup);
  }

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSNR_RevA) {

    grid_ui_model_init(ui, 14 + 1);

    for (uint8_t j = 0; j < 14 + 1; j++) {

      struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

      if (j < 8) {

        grid_ui_element_button_init(ele);

      } else if (j < 9) {

        grid_ui_element_endless_init(ele);

      } else if (j < 13) {

        grid_ui_element_button_init(ele);

      } else if (j < 14) {

        grid_ui_element_lcd_init(ele);
      } else {
        grid_ui_element_system_init(ele);
      }
    }

    ui->lua_ui_init_callback = grid_lua_ui_init_tek1;

  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA) {

    grid_platform_printf("VSN2\n");
    grid_ui_model_init(ui, 18 + 1);
    for (uint8_t j = 0; j < 18 + 1; j++) {

      struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

      if (j < 8) {

        grid_ui_element_button_init(ele);

      } else if (j < 12) {

        grid_ui_element_button_init(ele);

      } else if (j < 13) {

        grid_ui_element_lcd_init(ele);
      } else if (j < 17) {

        grid_ui_element_button_init(ele);

      } else if (j < 18) {

        grid_ui_element_lcd_init(ele);
      } else {
        grid_ui_element_system_init(ele);
      }
    }

    ui->lua_ui_init_callback = grid_lua_ui_init_vsn2;
  }
}

void app_main(void) {

  // set console baud rate
  ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_0, 2000000ul));

  esp_log_level_set("*", ESP_LOG_INFO);
  uint8_t loopcounter = 0;

  TaskHandle_t core2_task_hdl;
  xTaskCreatePinnedToCore(system_init_core_2_task, "swd_init", 1024 * 3, NULL, 4, &core2_task_hdl, 1);

  // grid_lcd_hardware_init();

  gpio_set_direction(16, GPIO_MODE_OUTPUT);
  gpio_set_level(16, 1);
  gpio_set_pull_mode(16, GPIO_PULLUP_ONLY);

  esp_log_level_set("*", ESP_LOG_INFO);

  SemaphoreHandle_t lua_busy_semaphore = xSemaphoreCreateBinary();
  SemaphoreHandle_t ui_busy_semaphore = xSemaphoreCreateBinary();

  void grid_common_semaphore_lock_fn(void* arg) {

    while (xSemaphoreTake((SemaphoreHandle_t)arg, 0) != pdTRUE) {
      // spin
      portYIELD();
    };
  }

  void grid_common_semaphore_release_fn(void* arg) { xSemaphoreGive((SemaphoreHandle_t)arg); }

  ESP_LOGI(TAG, "===== MAIN START =====");

  // size_t psram_size = esp_psram_get_size();
  // ESP_LOGI(TAG, "PSRAM size: %d bytes\n", psram_size);

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
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSNR_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA) {
    grid_module_tek1_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state, grid_sys_get_hwcfg(&grid_sys_state));
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PB44_RevA) {
    grid_module_pb44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else {
    ets_printf("Init Module: Unknown Module\r\n");
  }

  grid_ui_semaphore_init(&grid_ui_state, (void*)ui_busy_semaphore, grid_common_semaphore_lock_fn, grid_common_semaphore_release_fn);

  uint8_t led_pin = 21;

  grid_led_set_pin(&grid_led_state, led_pin);

  TaskHandle_t led_task_hdl;
  xTaskCreatePinnedToCore(grid_esp32_led_task, "led", 1024 * 3, NULL, LED_TASK_PRIORITY, &led_task_hdl, 0);

  TaskHandle_t usb_task_hdl;
  xTaskCreatePinnedToCore(grid_esp32_usb_task, "TinyUSB", 4096, NULL, 6, &usb_task_hdl, 1);

  // GRID MODULE INITIALIZATION SEQUENCE

  ESP_LOGI(TAG, "===== NVM START =====");
  grid_esp32_nvm_init(&grid_esp32_nvm_state);

  if (gpio_get_level(GRID_ESP32_PINS_MAPMODE) == 0) {

    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, 1000);
    grid_alert_all_set_frequency(&grid_led_state, 4);
    grid_esp32_nvm_erase(&grid_esp32_nvm_state);
    vTaskDelay(pdMS_TO_TICKS(600));
  }

  ESP_LOGI(TAG, "===== MSG START =====");
  grid_msg_init(&grid_msg_state); // setup session id, last message buffer init

  ESP_LOGI(TAG, "===== LUA INIT =====");
  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_lua_semaphore_init(&grid_lua_state, (void*)lua_busy_semaphore, grid_common_semaphore_lock_fn, grid_common_semaphore_release_fn);

  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb

  // ================== START: grid_module_pbf4_init() ================== //

  ESP_LOGI(TAG, "===== PORT INIT =====");
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t types[6] = {GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_UI, GRID_PORT_TYPE_USB};
    uint8_t directions[6] = {GRID_CONST_NORTH, GRID_CONST_EAST, GRID_CONST_SOUTH, GRID_CONST_WEST, 0, 0};
    grid_port_init(&grid_port_array[i], types[i], directions[i]);

    grid_buffer_tx_array[i].buffer_length = sizeof(grid_buffer_tx_memory_array[i]);
    grid_buffer_tx_array[i].buffer_storage = grid_buffer_tx_memory_array[i];
    grid_buffer_init(&grid_buffer_tx_array[i], sizeof(grid_buffer_tx_memory_array[i]));

    grid_buffer_rx_array[i].buffer_length = sizeof(grid_buffer_rx_memory_array[i]);
    grid_buffer_rx_array[i].buffer_storage = grid_buffer_rx_memory_array[i];
    grid_buffer_init(&grid_buffer_rx_array[i], sizeof(grid_buffer_rx_memory_array[i]));

    grid_doublebuffer_tx_array[i].buffer_size = 0;
    grid_doublebuffer_tx_array[i].buffer_storage = NULL;

    if (i < 4) {
      // UART
      grid_doublebuffer_tx_array[i].buffer_size = sizeof(grid_doublebuffer_tx_memory_array[i]);
      grid_doublebuffer_tx_array[i].buffer_storage = &grid_doublebuffer_tx_memory_array[i];
    }

    if (i == 5) {
      // USB

      grid_doublebuffer_tx_array[i].buffer_size = sizeof(grid_doublebuffer_tx_memory_array[i - 1]);
      grid_doublebuffer_tx_array[i].buffer_storage = &grid_doublebuffer_tx_memory_array[i - 1];

      grid_doublebuffer_rx_array[i].buffer_size = sizeof(grid_doublebuffer_rx_memory_array[0]);
      grid_doublebuffer_rx_array[i].buffer_storage = &grid_doublebuffer_rx_memory_array[0];
    }
  }

  grid_transport_init(&grid_transport_state);

  grid_transport_register_port(&grid_transport_state, &grid_port_array[0]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[1]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[2]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[3]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[4]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[5]);

  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[0], &grid_buffer_rx_array[0]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[1], &grid_buffer_rx_array[1]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[2], &grid_buffer_rx_array[2]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[3], &grid_buffer_rx_array[3]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[4], &grid_buffer_rx_array[4]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[5], &grid_buffer_rx_array[5]);

  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[0], NULL);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[1], NULL);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[2], NULL);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[3], NULL);
  grid_transport_register_doublebuffer(&grid_transport_state, NULL, NULL);                                                     // UI
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[5], &grid_doublebuffer_rx_array[5]); // USB

  ESP_LOGI(TAG, "===== BANK INIT =====");
  grid_sys_set_bank(&grid_sys_state, 0);
  ets_delay_us(2000);

  check_heap();
  xSemaphoreGive(ui_busy_semaphore);

  if (grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_TEK1_RevA) {
  }
  grid_ui_page_load(&grid_ui_state, 0); // load page 0
  SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  ESP_LOGI(TAG, "===== UI TASK INIT =====");

  check_heap();

  TaskHandle_t module_task_hdl;
  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_po16_task, "po16", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_bu16_task, "bu16", 1024 * 3, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_pbf4_task, "pbf4", 1024 * 3, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_en16_task, "en16", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD) {
    xTaskCreatePinnedToCore(grid_esp32_module_ef44_task, "ef44", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK2_RevA) {
    xTaskCreatePinnedToCore(grid_esp32_module_tek2_task, "tek2", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN1_RevA ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSNR_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_VSN2_RevA) {
    xTaskCreatePinnedToCore(grid_esp32_module_tek1_task, "tek1", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PB44_RevA) {
    xTaskCreatePinnedToCore(grid_esp32_module_pb44_task, "pb44", 1024 * 3, NULL, MODULE_TASK_PRIORITY, &module_task_hdl, 0);
  } else {
    printf("Init Module: Unknown Module\r\n");
  }

  ESP_LOGI(TAG, "===== UI TASK DONE =====");

  grid_ui_state.ui_interaction_enabled = 1;
  // ================== FINISH: grid_module_pbf4_init() ================== //

  // Create the class driver task

  TaskHandle_t nvm_task_hdl;

  TaskHandle_t port_task_hdl;
  TaskHandle_t ui_task_hdl;

  xTaskCreatePinnedToCore(grid_esp32_port_task, "port", 4096 * 10, NULL, PORT_TASK_PRIORITY, &port_task_hdl, 1);

  ESP_LOGI(TAG, "===== PORT TASK DONE =====");

  // Create the class driver task

  xTaskCreatePinnedToCore(grid_esp32_nvm_task, "nvm", 1024 * 10, NULL, NVM_TASK_PRIORITY, &nvm_task_hdl, 0);

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

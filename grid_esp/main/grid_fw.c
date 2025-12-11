/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// must be last freertos relevant header to avoid #error
#include "esp_freertos_hooks.h"

#include "grid_allocator.h"
#include "grid_platform.h"
#include "grid_transport.h"

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
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_task_wdt.h"

#include "esp_private/esp_psram_extram.h"

#include "grid_esp32_led.h"
#include "grid_esp32_module_bu16.h"
#include "grid_esp32_module_ef44.h"
#include "grid_esp32_module_en16.h"
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

#include "vmp_def.h"
#include "vmp_tag.h"

static const char* TAG = "main";

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static bool periodic_rtc_ms_cb() {

  grid_ui_rtc_ms_tick_time(&grid_ui_state);

  if (gpio_get_level(GRID_ESP32_PINS_MAPMODE)) {
    grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 0);
  } else {
    grid_ui_rtc_ms_mapmode_handler(&grid_ui_state, 1);
  }

  return false;
}

static void periodic_rtc_ms_init() {

  gptimer_handle_t tmr = NULL;
  gptimer_config_t tmr_cfg = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1000000,
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&tmr_cfg, &tmr));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = periodic_rtc_ms_cb,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(tmr, &cbs, NULL));

  gptimer_alarm_config_t alarm_cfg = {
      .reload_count = 0,
      .alarm_count = 1000,
      .flags.auto_reload_on_alarm = true,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(tmr, &alarm_cfg));

  ESP_ERROR_CHECK(gptimer_enable(tmr));
  ESP_ERROR_CHECK(gptimer_start(tmr));
}

void system_init_core_2_task(void* arg) {

  grid_esp32_swd_pico_pins_init(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, GRID_ESP32_PINS_RP_CLOCK);
  grid_esp32_swd_pico_clock_init(LEDC_TIMER_0, LEDC_CHANNEL_0);
  grid_esp32_swd_pico_program_sram(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, pico_firmware, pico_firmware_len);

  vTaskDelete(NULL);
}

bool idle_hook(void) {

  portYIELD();
  return 0;
}

static void log_checkpoint(const char* str) {

  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  int integrity = heap_caps_check_integrity_all(true);
  size_t highwater = uxTaskGetStackHighWaterMark(NULL);
  ESP_LOGI(TAG, "===== %s, free mem: %u (integrity: %d), highwater: %u", str, free_size, integrity, highwater);
  if (highwater < 512) {
    ESP_LOGE(TAG, "highwatermark low: %u < 512", highwater);
  }
}

#include "grid_ui_lcd.h"

void grid_ui_element_lcd_template_parameter_init_vsn_left(struct grid_ui_template_buffer* buf) {

  grid_ui_element_lcd_template_parameter_init(buf);

  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_L_SCREEN_INDEX_index] = 0;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_WIDTH_index] = 320;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_HEIGHT_index] = 240;
}

void grid_ui_element_lcd_template_parameter_init_vsn_right(struct grid_ui_template_buffer* buf) {

  grid_ui_element_lcd_template_parameter_init(buf);

  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_L_SCREEN_INDEX_index] = 1;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_WIDTH_index] = 320;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_HEIGHT_index] = 240;
}

void grid_module_tek1_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui, struct grid_sys_model* sys) {

  bool is_tek1_reva = grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_TEK1_RevA;

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 4);  // TODO: 12 ain for TEK2
  grid_led_init(led, 13 + 5); // TODO: 18 led for TEK2

  if (is_tek1_reva) {

    // TODO

  } else if (grid_hwcfg_module_is_vsnl(&grid_sys_state)) {

    for (uint8_t i = 0; i < 8; ++i) {
      grid_led_lookup_alloc_single(led, i, i + 10);
    }
    grid_led_lookup_alloc_multi(led, 8, 5, (uint8_t[5]){5, 6, 7, 8, 9});

  } else if (grid_hwcfg_module_is_vsnr(&grid_sys_state)) {

    for (uint8_t i = 0; i < 8; ++i) {
      grid_led_lookup_alloc_single(led, i, i + 10);
    }
    grid_led_lookup_alloc_multi(led, 8, 5, (uint8_t[5]){0, 1, 2, 3, 4});

  } else if (grid_hwcfg_module_is_vsn2(&grid_sys_state)) {

    for (uint8_t i = 0; i < 8; ++i) {
      grid_led_lookup_alloc_single(led, i, i + 10);
    }
  }

  if (grid_hwcfg_module_is_vsnl(&grid_sys_state) || is_tek1_reva) {

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

        grid_ui_element_lcd_init(ele, grid_ui_element_lcd_template_parameter_init_vsn_left);
      } else {
        grid_ui_element_system_init(ele);
      }
    }

  } else if (grid_hwcfg_module_is_vsnr(&grid_sys_state)) {

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

        grid_ui_element_lcd_init(ele, grid_ui_element_lcd_template_parameter_init_vsn_right);
      } else {
        grid_ui_element_system_init(ele);
      }
    }

  } else if (grid_hwcfg_module_is_vsn2(&grid_sys_state)) {

    grid_ui_model_init(ui, 18 + 1);
    for (uint8_t j = 0; j < 18 + 1; j++) {

      struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

      if (j < 8) {

        grid_ui_element_button_init(ele);

      } else if (j < 12) {

        grid_ui_element_button_init(ele);

      } else if (j < 13) {

        grid_ui_element_lcd_init(ele, grid_ui_element_lcd_template_parameter_init_vsn_left);
      } else if (j < 17) {

        grid_ui_element_button_init(ele);

      } else if (j < 18) {

        grid_ui_element_lcd_init(ele, grid_ui_element_lcd_template_parameter_init_vsn_right);
      } else {
        grid_ui_element_system_init(ele);
      }
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init;
}

void grid_esp32_print_chip_info() {

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  ESP_LOGI(TAG, "This is %s chip, ", CONFIG_IDF_TARGET);

  ESP_LOGI(TAG, "with %d cores, ", chip_info.cores);

  char* has_bt = chip_info.features & CHIP_FEATURE_BT ? "/BT" : "";
  char* has_ble = chip_info.features & CHIP_FEATURE_BLE ? "/BLE" : "";
  ESP_LOGI(TAG, "WiFi%s%s, ", has_bt, has_ble);

  ESP_LOGI(TAG, "silicon rev %d, ", chip_info.revision);

  uint32_t size_flash_chip = 0;
  esp_flash_get_size(NULL, &size_flash_chip);
  char* flash_location = chip_info.features & CHIP_FEATURE_EMB_FLASH ? "embedded" : "external";
  ESP_LOGI(TAG, "%uMB %s flash, ", (unsigned int)size_flash_chip >> 20, flash_location);

  ESP_LOGI(TAG, "free heap: %u.", (unsigned int)esp_get_free_heap_size());
}

void app_main(void) {

  // Allocate profiler & assign its interface
  vmp_buf_malloc(&vmp, 100, sizeof(struct vmp_evt_t));
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_serialize = vmp_evt_serialize,
      .fwrite = vmp_fwrite,
  };
  bool vmp_flushed = false;

  // Configure task timers
  struct grid_utask_timer timer_led = (struct grid_utask_timer){
      .last = grid_platform_rtc_get_micros(),
      .period = 9500, // 10000 really, but FreeRTOS is currently 100 Hz
  };

  // set console baud rate
  ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_0, 2000000ul));
  vTaskDelay(1);

  esp_log_level_set("*", ESP_LOG_INFO);

  TaskHandle_t core2_task_hdl;
  xTaskCreatePinnedToCore(system_init_core_2_task, "swd_init", 1024 * 3, NULL, 4, &core2_task_hdl, 1);

  // grid_lcd_hardware_init();

  gpio_set_direction(16, GPIO_MODE_OUTPUT);
  gpio_set_level(16, 1);
  gpio_set_pull_mode(16, GPIO_PULLUP_ONLY);

  esp_log_level_set("*", ESP_LOG_INFO);

  SemaphoreHandle_t lua_busy_semaphore = xSemaphoreCreateBinary();
  SemaphoreHandle_t ui_busy_semaphore = xSemaphoreCreateBinary();
  SemaphoreHandle_t ui_bulk_semaphore = xSemaphoreCreateBinary();

  void grid_common_semaphore_lock_fn(void* arg) {

    while (xSemaphoreTake((SemaphoreHandle_t)arg, 0) != pdTRUE) {
      // spin
      portYIELD();
    };
  }

  void grid_common_semaphore_release_fn(void* arg) { xSemaphoreGive((SemaphoreHandle_t)arg); }

  grid_platform_printf("");
  log_checkpoint("MAIN START");

  grid_esp32_print_chip_info();

  gpio_set_direction(GRID_ESP32_PINS_MAPMODE, GPIO_MODE_INPUT);
  gpio_pullup_en(GRID_ESP32_PINS_MAPMODE);

  log_checkpoint("SYS START");
  grid_sys_init(&grid_sys_state);

  log_checkpoint("PSRAM INIT");
  if (grid_hwcfg_module_is_vsnx(&grid_sys_state)) {

    log_checkpoint("PSRAM SCREEN MODULE");

    ESP_ERROR_CHECK(esp_psram_init());

    esp_err_t r = esp_psram_extram_add_to_heap_allocator();
    if (r != ESP_OK) {
      ESP_LOGE(TAG, "app_main() failed to add external RAM to heap");
      abort();
    }
  }

  size_t psram_size = esp_psram_get_size();
  ESP_LOGI(TAG, "PSRAM size: %d bytes\n", psram_size);

  log_checkpoint("UI INIT");
  if (grid_hwcfg_module_is_po16(&grid_sys_state)) {
    grid_module_po16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_pbf4(&grid_sys_state)) {
    grid_module_pbf4_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_en16(&grid_sys_state)) {
    grid_module_en16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_tek2(&grid_sys_state)) {
    grid_module_tek2_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  } else if (grid_hwcfg_module_is_vsnx(&grid_sys_state)) {
    grid_module_tek1_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state, &grid_sys_state);
  } else {
    ets_printf("UI Init failed: Unknown Module\r\n");
  }

  grid_ui_semaphore_init(&grid_ui_state.busy_semaphore, (void*)ui_busy_semaphore, grid_common_semaphore_lock_fn, grid_common_semaphore_release_fn);
  grid_ui_semaphore_init(&grid_ui_state.bulk_semaphore, (void*)ui_bulk_semaphore, grid_common_semaphore_lock_fn, grid_common_semaphore_release_fn);

  grid_led_set_pin(&grid_led_state, 21);
  grid_esp32_led_start(grid_led_get_pin(&grid_led_state));

  grid_esp32_usb_init();
  grid_usb_midi_buffer_init();
  grid_usb_keyboard_model_init(&grid_usb_keyboard_state, 100);

  // GRID MODULE INITIALIZATION SEQUENCE

  log_checkpoint("NVM START");
  grid_esp32_nvm_mount(&grid_esp32_nvm_state, false);

  if (gpio_get_level(GRID_ESP32_PINS_MAPMODE) == 0) {

    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, 1000);
    grid_alert_all_set_frequency(&grid_led_state, 4);
    grid_platform_nvm_format_and_mount();
    grid_esp32_utask_led(&timer_led);
    vTaskDelay(pdMS_TO_TICKS(500));
    grid_esp32_utask_led(&timer_led);
  }

  log_checkpoint("MSG START");
  grid_msg_model_init(&grid_msg_state);

  log_checkpoint("LUA INIT");
  grid_lua_init(&grid_lua_state, NULL, NULL);
  grid_lua_semaphore_init(&grid_lua_state, (void*)lua_busy_semaphore, grid_common_semaphore_lock_fn, grid_common_semaphore_release_fn);

  grid_lua_set_memory_target(&grid_lua_state, 100);

  // ================== START: grid_module_pbf4_init() ================== //

  const int PORT_COUNT = 6;
  grid_transport_malloc(&grid_transport_state, PORT_COUNT);

  grid_port_init(&grid_transport_state.ports[0], GRID_PORT_USART, GRID_PORT_NORTH);
  grid_port_init(&grid_transport_state.ports[1], GRID_PORT_USART, GRID_PORT_EAST);
  grid_port_init(&grid_transport_state.ports[2], GRID_PORT_USART, GRID_PORT_SOUTH);
  grid_port_init(&grid_transport_state.ports[3], GRID_PORT_USART, GRID_PORT_WEST);
  grid_port_init(&grid_transport_state.ports[4], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[5], GRID_PORT_USB, 0);

  log_checkpoint("BANK INIT");
  grid_sys_set_bank(&grid_sys_state, 0);
  ets_delay_us(2000);

  xSemaphoreGive(ui_busy_semaphore);
  xSemaphoreGive(ui_bulk_semaphore);

  grid_ui_page_load(&grid_ui_state, 0); // load page 0
  SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  log_checkpoint("GUI INIT");

  uint32_t hwcfg = grid_sys_get_hwcfg(&grid_sys_state);

  // Initialize font
  if (grid_hwcfg_module_is_vsnx(&grid_sys_state)) {
    grid_font_init(&grid_font_state);
  }

  uint32_t width = LCD_HRES;
  uint32_t height = LCD_VRES;
  uint32_t size = width * height * GRID_GUI_BYTES_PPX;

  struct grid_gui_model* guis = grid_gui_states;
  struct grid_esp32_lcd_model* lcds = grid_esp32_lcd_states;

  // Initialize GUI at index 0, if necessary
  if (grid_hwcfg_module_has_lcd0(&grid_sys_state)) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[0], &lcds[0], buf, size, width, height);
    grid_gui_clear(&guis[0], grid_gui_color_from_rgb(0, 0, 0));
    grid_gui_swap_set(&guis[0], true);
  }

  // Initialize GUI panel at index 1, if necessary
  if (grid_hwcfg_module_has_lcd1(&grid_sys_state)) {
    uint8_t* buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    grid_gui_init(&guis[1], &lcds[1], buf, size, width, height);
    grid_gui_clear(&guis[1], grid_gui_color_from_rgb(0, 0, 0));
    grid_gui_swap_set(&guis[1], true);
  }

  log_checkpoint("NVM TASK INIT");

  TaskHandle_t nvm_task_hdl;

  xTaskCreatePinnedToCore(grid_esp32_nvm_task, "nvm", 1024 * 10, NULL, NVM_TASK_PRIORITY, &nvm_task_hdl, 0);

  log_checkpoint("MODULE INIT");

  if (grid_hwcfg_module_is_po16(&grid_sys_state)) {
    grid_esp32_module_po16_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_bu16(&grid_sys_state)) {
    grid_esp32_module_bu16_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_pbf4(&grid_sys_state)) {
    grid_esp32_module_pbf4_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_en16(&grid_sys_state)) {
    grid_esp32_module_en16_init(&grid_sys_state, &grid_ui_state, &grid_esp32_encoder_state);
  } else if (grid_hwcfg_module_is_ef44(&grid_sys_state)) {
    grid_esp32_module_ef44_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_esp32_encoder_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_tek2(&grid_sys_state)) {
    grid_esp32_module_tek2_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_config_state, &grid_cal_state);
  } else if (grid_hwcfg_module_is_vsnx(&grid_sys_state)) {
    grid_esp32_module_tek1_init(&grid_sys_state, &grid_ui_state, &grid_esp32_adc_state, &grid_config_state, &grid_cal_state, grid_esp32_lcd_states);
  } else {
    ets_printf("Task Init failed: Unknown Module\r\n");
  }

  log_checkpoint("UI TASK DONE");

  if (grid_hwcfg_module_is_vsnx(&grid_sys_state)) {

    TaskHandle_t lcd_task_hdl;

    xTaskCreatePinnedToCore(grid_esp32_lcd_task, "lcd", 1024 * 4, NULL, MODULE_TASK_PRIORITY, &lcd_task_hdl, 0);

    log_checkpoint("LCD TASK DONE");
  }

  TaskHandle_t port_task_hdl;

  xTaskCreatePinnedToCore(grid_esp32_port_task, "port", 1024 * 10, NULL, PORT_TASK_PRIORITY, &port_task_hdl, 1);

  log_checkpoint("PORT TASK DONE");

  // Initialize 1 kHz timer
  periodic_rtc_ms_init();

  log_checkpoint("INIT COMPLETE");

  // Register idle hook to force yield from idle task to lowest priority task
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 0);
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 1);

  // log_checkpoint("TRACE START");

  // TRACE CONFIG GPIO 40, 41, 4Mbaud,

  // esp_apptrace_init();

  // SEGGER_SYSVIEW_RecordSystime();
  // SEGGER_SYSVIEW_SendTaskList();
  // SEGGER_SYSVIEW_SendNumModules();

  // SEGGER_SYSVIEW_Start();

  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
  gpio_pullup_en(GPIO_NUM_0);

  log_checkpoint("MAIN LOOP");

  while (1) {

    // Flush the profiler output if it becomes full
    if (!vmp_flushed && vmp.size == vmp.capacity) {

      portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
      portENTER_CRITICAL(&spinlock);

      vmp_serialize_start(&reg);
      vmp_buf_serialize_and_write(&vmp, &reg);
      vmp_uid_str_serialize_and_write(VMP_UID_COUNT, VMP_ASSOC, &reg);
      vmp_serialize_close(&reg);

      portEXIT_CRITICAL(&spinlock);

      vmp_flushed = true;
    }

    // Run microtasks
    grid_esp32_utask_led(&timer_led);

    vTaskDelay(1);
  }

  // Deallocate profiler
  vmp_buf_free(&vmp);

  ESP_LOGI(TAG, "===== MAIN COMPLETE =====");
}

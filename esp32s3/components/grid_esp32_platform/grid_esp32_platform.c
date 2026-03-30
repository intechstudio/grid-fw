/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_platform.h"

#include "esp_heap_caps.h"

#include "rom/ets_sys.h" // For ets_printf

#include "esp_timer.h"

// #include "hal/cpu_hal.h"

#include "esp_cpu.h"
#include "esp_efuse.h"
#include "esp_log.h"
#include "esp_random.h"

#include "driver/gpio.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_pins.h"

static const char* TAG = "grid_esp32_platform";

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // ets_printf("ADDRESS: %lx\r\n", handle);

  if (handle == NULL) {

    ets_printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}

uint64_t IRAM_ATTR grid_platform_rtc_get_micros(void) { return esp_timer_get_time(); }

uint64_t IRAM_ATTR grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return ((t1 << 1) - (t2 << 1)) >> 1; }

uint64_t IRAM_ATTR grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_diff(grid_platform_rtc_get_micros(), told); }

uint32_t IRAM_ATTR grid_platform_get_cycles() { return esp_cpu_get_cycle_count(); }

uint32_t IRAM_ATTR grid_platform_get_cycles_per_us() { return CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ; }

static char uint4_to_hex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static void uint8_to_hex(uint8_t u, char h[2]) {
  h[0] = uint4_to_hex[u >> 4];
  h[1] = uint4_to_hex[u & 0xf];
}

void IRAM_ATTR grid_platform_printf_nonprint(const uint8_t* src, size_t size) {

  for (size_t i = 0; i < size; ++i) {

    if (src[i] < 32) {

      char hex[2];
      uint8_to_hex(src[i], hex);
      ets_printf("[%c%c]", hex[0], hex[1]);

    } else {

      ets_printf("%c", src[i]);
    }
  }
}

uint32_t grid_platform_get_hwcfg_bit(uint8_t n) {

  gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA, GPIO_MODE_INPUT);

  gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
  gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);

  ets_delay_us(40);

  gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1);

  ets_delay_us(10);

  uint8_t level = 0;
  for (uint8_t i = 0; i < n + 1; ++i) {

    gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);

    level = gpio_get_level(GRID_ESP32_PINS_HWCFG_DATA);

    ets_delay_us(10);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);
    ets_delay_us(10);
  }

  return level > 0;
}

uint32_t grid_platform_get_hwcfg() {

  gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA, GPIO_MODE_INPUT);

  gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
  gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);

  ets_delay_us(40);

  gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1);

  ets_delay_us(10);

  uint8_t hwcfg = 0;
  for (uint8_t i = 0; i < 8; ++i) {

    gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);

    uint8_t level = gpio_get_level(GRID_ESP32_PINS_HWCFG_DATA);
    hwcfg |= ((level > 0) << i);

    ets_delay_us(10);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);
    ets_delay_us(10);
  }

  ESP_LOGI(TAG, "HWCFG value: %d", hwcfg);
  return hwcfg;
}

uint32_t grid_platform_get_id(uint32_t* return_array) {

  /*

      struct ESP_FUSE3
      {
          uint8_t crc;
          uint8_t macAddr[6];
          uint8_t reserved[8];
          uint8_t version;
      };
  */

  uint8_t block[32] = {0};

  if (ESP_OK == esp_efuse_read_block(EFUSE_BLK1, block, 0, 6 * 8)) {
    ESP_LOGI(TAG, "CPUID OK");
  }

  uint8_t* mac_address = &block[0];

  ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

  uint64_t cpuid = 0;

  for (uint8_t i = 0; i < 6; i++) {

    // ESP_LOGI(TAG, "CPUID: %016llx",cpuid);
    cpuid |= ((uint64_t)mac_address[i]) << ((5 - i) * 8);
  }

  ESP_LOGI(TAG, "CPUID: %016llx", cpuid);

  uint8_t* array = (uint8_t*)return_array;
  array[0] = mac_address[0];
  array[1] = mac_address[1];
  array[2] = mac_address[2];
  array[3] = mac_address[3];
  array[4] = mac_address[4];
  array[5] = mac_address[5];

  return 0;
}

uint8_t grid_platform_get_random_8() {
  uint32_t random_number = esp_random();
  return random_number % 256;
}

void grid_platform_delay_ms(uint32_t delay_milliseconds) { ets_delay_us(delay_milliseconds * 1000); }

void grid_platform_delay_us(uint32_t delay_microseconds) { ets_delay_us(delay_microseconds); }

uint8_t grid_platform_get_reset_cause() { return 0; }

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;

  char temp[1012] = {0};

  va_start(ap, fmt);

  vsnprintf(temp, 1012, fmt, ap);

  va_end(ap);

  ets_printf(temp);
}

uint8_t grid_platform_disable_grid_transmitter(uint8_t direction) {

  ets_printf("grid_platform_disable_grid_transmitter NOT IMPLEMENTED!!!\r\n");
  return 1;
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction) {

  // ets_printf("grid_platform_reset_grid_transmitter NOT IMPLEMENTED!!!\r\n");
  return 1;
}

uint8_t grid_platform_enable_grid_transmitter(uint8_t direction) {

  ets_printf("grid_platform_enable_grid_transmitter NOT IMPLEMENTED!!!\r\n");
  return 1;
}

void grid_platform_system_reset() { ets_printf("grid_platform_system_reset NOT IMPLEMENTED!!!\r\n"); }

uint8_t IRAM_ATTR grid_platform_get_adc_bit_depth() { return 12; }

void grid_platform_mux_init(uint8_t mux_positions_bm) { grid_esp32_adc_mux_init(&grid_esp32_adc_state, mux_positions_bm); }

void IRAM_ATTR grid_platform_mux_write(uint8_t index) {

  grid_esp32_adc_state.mux_index = index;
  grid_esp32_adc_mux_update(&grid_esp32_adc_state);
}

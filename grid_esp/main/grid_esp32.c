/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32.h"

static const char* TAG = "grid_esp32";

void vTaskGetRunTimeStats2(char* pcWriteBuffer) {

  TaskStatus_t* pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  uint32_t ulTotalRunTime, ulStatsAsPercentage;

  // Make sure the write buffer does not contain a string.
  *pcWriteBuffer = 0x00;

  // Take a snapshot of the number of tasks in case it changes while this
  // function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate a TaskStatus_t structure for each task.  An array could be
  // allocated statically at compile time.
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL) {
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    // grid_platform_printf("Task Count : %d Core: %d\r\n\r\n", uxArraySize,
    // xPortGetCoreID());

    // Avoid divide by zero errors.
    if (ulTotalRunTime > 0) {
      // For each populated position in the pxTaskStatusArray array,
      // format the raw data as human readable ASCII data
      for (x = 0; x < uxArraySize; x++) {

        char taskName[10] = ".........\0";
        snprintf(taskName, 6, pxTaskStatusArray[x].pcTaskName);

        uint8_t core = xTaskGetAffinity(pxTaskStatusArray[x].xHandle);

        uint8_t priority = uxTaskPriorityGet(pxTaskStatusArray[x].xHandle);

        /* Inspect our own high water mark on entering the task. */
        unsigned long uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);

        // What percentage of the total run time has the task used?
        // This will always be rounded down to the nearest integer.
        // ulTotalRunTimeDiv100 has already been divided by 100.
        ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / (ulTotalRunTime / 100);

        uint32_t runtime = pxTaskStatusArray[x].ulRunTimeCounter;

        TaskHandle_t task = pxTaskStatusArray[x].xHandle;

        char core_char = 'X';

        if (core == 0) {
          core_char = '0';
        } else if (core == 1) {
          core_char = '1';
        }

        sprintf(pcWriteBuffer, "%c-%s\t\t0x%lx\t\t%lu\t\t%d\t\t%lu pcnt (%lu/%lu)\r\n", core_char, taskName, (unsigned long int)pxTaskStatusArray[x].xHandle, uxHighWaterMark, priority,
                ulStatsAsPercentage, runtime, ulTotalRunTime);

        pcWriteBuffer += strlen((char*)pcWriteBuffer);
      }
    }

    // The array is no longer needed, free the memory it consumes.
    vPortFree(pxTaskStatusArray);
  }
}

#define MAX_TASK_ID 16

uint32_t lastRunTimeCounter[MAX_TASK_ID] = {0};
uint32_t lastTotalRunTime = 0;

uint8_t skip_list[MAX_TASK_ID] = {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0};

void vTaskGetRunTimeStats3(char* pcWriteBuffer) {

  TaskStatus_t* pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  uint32_t ulTotalRunTime, ulStatsAsPercentage;

  // Make sure the write buffer does not contain a string.
  *pcWriteBuffer = 0x00;

  // Take a snapshot of the number of tasks in case it changes while this
  // function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate a TaskStatus_t structure for each task.  An array could be
  // allocated statically at compile time.
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL) {
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    // grid_platform_printf("Task Count : %d Core: %d\r\n\r\n", uxArraySize,
    // xPortGetCoreID());

    // Avoid divide by zero errors.
    if (ulTotalRunTime > 0) {
      // For each populated position in the pxTaskStatusArray array,
      // format the raw data as human readable ASCII data

      sprintf(pcWriteBuffer, "{");

      for (uint8_t i = 0; i < MAX_TASK_ID; i++) {

        for (x = 0; x < uxArraySize; x++) {

          uint32_t taskNumber = pxTaskStatusArray[x].xTaskNumber;

          if (taskNumber == i) {

            char taskName[10] = ".........\0";
            snprintf(taskName, 6, pxTaskStatusArray[x].pcTaskName);

            uint8_t core = xTaskGetAffinity(pxTaskStatusArray[x].xHandle);

            /* Inspect our own high water mark on entering the task. */
            unsigned long uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);

            // What percentage of the total run time has the task used?
            // This will always be rounded down to the nearest integer.

            uint32_t taskElapsedTime = pxTaskStatusArray[x].ulRunTimeCounter - lastRunTimeCounter[taskNumber];
            uint32_t totalElapsedTime = (ulTotalRunTime - lastTotalRunTime);

            ulStatsAsPercentage = (taskElapsedTime * 100) / (totalElapsedTime);

            lastRunTimeCounter[taskNumber] = pxTaskStatusArray[x].ulRunTimeCounter;

            uint32_t runtime = pxTaskStatusArray[x].ulRunTimeCounter;

            TaskHandle_t task = pxTaskStatusArray[x].xHandle;

            char core_char = 'X';

            if (core == 0) {
              core_char = '0';
            } else if (core == 1) {
              core_char = '1';
            }

            if (skip_list[i] == 0) {

              uint32_t debug_var = ulStatsAsPercentage;

              // As Percentage (string)
              sprintf(pcWriteBuffer, "\"c%c %02lu %s %lx\": \"%lu%%%%\", ", core_char, taskNumber, taskName, (long unsigned int)pxTaskStatusArray[x].xHandle, debug_var);

              // As Integer (string)
              // sprintf( pcWriteBuffer, "\"c%c %02lu %s\": \"%lu\", ",
              // core_char, taskNumber,  taskName, debug_var);

              // As Integer (number)
              // sprintf( pcWriteBuffer, "\"c%c %02lu %s\": %lu, ", core_char,
              // taskNumber,  taskName, debug_var);
            } else {

              // first run
              if (lastTotalRunTime == 0) {

                ets_printf("SKIPLIST: %s\r\n", taskName);
              }
            }

            pcWriteBuffer += strlen((char*)pcWriteBuffer);
          }
        }
      }

      sprintf(&pcWriteBuffer[strlen(pcWriteBuffer) - 2], "}");
      lastTotalRunTime = ulTotalRunTime;
    }

    // The array is no longer needed, free the memory it consumes.
    vPortFree(pxTaskStatusArray);
  }
}

void grid_esp32_housekeeping_task(void* arg) {

  char stats[3000] = {0};

  int8_t axis_psition = 0;

  while (1) {

    // grid_platform_usb_gamepad_axis_move(axis_psition, GAMEPAD_AXIS_X);
    // axis_psition++;

    // vTaskGetRunTimeStats2(stats);

    // grid_port_debug_print_text(stats);

    // ets_printf("%s\r\n", stats);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Wait to be deleted
  vTaskSuspend(NULL);
}

uint32_t grid_platform_get_hwcfg() {

  gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
  gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA, GPIO_MODE_INPUT);

  gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
  gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);

  ets_delay_us(1000);

  uint8_t hwcfg_value = 0;

  for (uint8_t i = 0; i < 8; i++) { // now we need to shift in the remaining 7 values

    // SHIFT DATA
    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT,
                   1); // This outputs the first value to HWCFG_DATA
    ets_delay_us(1000);

    if (gpio_get_level(GRID_ESP32_PINS_HWCFG_DATA)) {

      hwcfg_value |= (1 << i);
    } else {
    }

    if (i != 7) {

      // Clock rise
      gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);

      ets_delay_us(1000);

      gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);
    }
  }

  ESP_LOGI(TAG, "HWCFG value: %d", hwcfg_value);
  return hwcfg_value;
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

uint8_t grid_platform_get_reset_cause() { return 0; }

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;

  char temp[200] = {0};

  va_start(ap, fmt);

  vsnprintf(temp, 199, fmt, ap);

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

void grid_platform_nvm_defrag() { ets_printf("grid_platform_nvm_defrag NOT IMPLEMENTED!!!\r\n"); }

uint8_t grid_platform_get_adc_bit_depth() { return 12; }

uint64_t grid_platform_rtc_get_micros(void) { return esp_timer_get_time(); }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_micros() - told; }

uint32_t grid_platform_get_cycles() { return cpu_hal_get_cycle_count(); }

uint32_t grid_platform_get_cycles_per_us() { return 240; }

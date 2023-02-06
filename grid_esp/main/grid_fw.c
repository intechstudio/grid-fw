/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"

#include <string.h>

#include "driver/spi_master.h"
#include "rom/ets_sys.h" // For ets_printf

#include "driver/gpio.h"
#include "esp_flash.h"
#include "esp_task_wdt.h"


#include "pico_firmware.h"
#include "grid_esp32_adc.h"
#include "grid_esp32_led.h"


#define ADC_TASK_PRIORITY 2
#define LED_TASK_PRIORITY 2
#define PORT_TASK_PRIORITY 3


#include "driver/ledc.h"
#include <esp_timer.h>

#include "grid_esp32.h"
#include "grid_esp32_swd.h"
#include "grid_esp32_port.h"



#include "esp_log.h"
#include "esp_check.h"
#include "rom/ets_sys.h" // For ets_printf


#include "../../grid_common/grid_protocol.h"
#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_led.h"
#include "../../grid_common/grid_sys.h"
#include "../../grid_common/grid_msg.h"
#include "../../grid_common/grid_buf.h"
#include "../../grid_common/grid_port.h"
#include "../../grid_common/grid_usb.h"
#include "../../grid_common/grid_module.h"

#include "../../grid_common/grid_lua_api.h"
#include "../../grid_common/grid_ui.h"


#include "../../grid_common/lua-5.4.3/src/lua.h"
#include "../../grid_common/lua-5.4.3/src/lualib.h"
#include "../../grid_common/lua-5.4.3/src/lauxlib.h"


#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static const char *TAG = "USB example";

static uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

volatile uint16_t grid_usb_rx_double_buffer_index = 0;

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);


    for (uint16_t i=0; i<rx_size; i++){

		GRID_PORT_H.rx_double_buffer[grid_usb_rx_double_buffer_index] = buf[i];

		
		grid_usb_rx_double_buffer_index++;
		grid_usb_rx_double_buffer_index%=GRID_DOUBLE_BUFFER_RX_SIZE;

	}

    //ESP_LOGI(TAG, "Data from channel %d len: %d", itf, rx_size);

}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}


//------------- CLASS -------------//
#define CFG_TUD_CDC             1
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            1
#define CFG_TUD_MSC             0
#define CFG_TUD_VENDOR          0


/** Helper defines **/

// Interface counter
enum interface_count {
#if CFG_TUD_MIDI
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,
#endif
#if CFG_TUD_CDC
    ITF_NUM_CDC_NOTIFY,
    ITF_NUM_CDC_DATA,
#endif
    ITF_COUNT
};

// USB Endpoint numbers
enum usb_endpoints {
    // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
    EP_EMPTY = 0,
#if CFG_TUD_MIDI
    EPNUM_MIDI,
#endif
#if CFG_TUD_CDC
    EPNUM_CDC_NOTIFY,
    EPNUM_CDC_DATA,
#endif
};

//#EPNUM_MIDI_IN
//#EPNUM_MIDI_OUT


/** TinyUSB descriptors **/

#define TUSB_DESCRIPTOR_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN )

/**
 * @brief String descriptor
 */
static const char* s_str_desc[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "Intech Studio",             // 1: Manufacturer
    "Grid",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Intech Grid MIDI device", // 4: MIDI
    "Intech Grid CDC device",  // 5: CDC
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and a MIDI interface
 */
static uint8_t s_cfg_desc[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 100),


    #if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NOTIFY, 5, (0x80 | EPNUM_CDC_NOTIFY), 64, EPNUM_CDC_DATA, (0x80 | EPNUM_CDC_DATA), 64),
    #endif

    #if CFG_TUD_MIDI
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
    #endif   
  
    


};


#if CFG_TUD_MIDI

static void midi_task_read_example(void *arg)
{
    // The MIDI interface always creates input and output port/jack descriptors
    // regardless of these being used or not. Therefore incoming traffic should be read
    // (possibly just discarded) to avoid the sender blocking in IO
    uint8_t packet[4];
    bool read = false;
    for (;;) {
        vTaskDelay(1);
        while (tud_midi_available()) {
            read = tud_midi_packet_read(packet);
            if (read) {
                ESP_LOGI(TAG, "Read - Time (ms since boot): %lld, Data: %02hhX %02hhX %02hhX %02hhX",
                         esp_timer_get_time(), packet[0], packet[1], packet[2], packet[3]);
            }
        }
    }
}

static void periodic_midi_write_example_cb(void *arg)
{


    ESP_LOGI(TAG, "Periodic Midi Task \r\n");
    
    // Example melody stored as an array of note values
    uint8_t const note_sequence[] = {
        74, 78, 81, 86, 90, 93, 98, 102, 57, 61, 66, 69, 73, 78, 81, 85, 88, 92, 97, 100, 97, 92, 88, 85, 81, 78,
        74, 69, 66, 62, 57, 62, 66, 69, 74, 78, 81, 86, 90, 93, 97, 102, 97, 93, 90, 85, 81, 78, 73, 68, 64, 61,
        56, 61, 64, 68, 74, 78, 81, 86, 90, 93, 98, 102
    };

    static uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
    static uint8_t const channel = 0; // 0 for channel 1
    static uint32_t note_pos = 0;

    // Previous positions in the note sequence.
    int previous = note_pos - 1;

    // If we currently are at position 0, set the
    // previous position to the last note in the sequence.
    if (previous < 0) {
        previous = sizeof(note_sequence) - 1;
    }

    // Send Note On for current position at full velocity (127) on channel 1.
    ESP_LOGI(TAG, "Writing MIDI data %d", note_sequence[note_pos]);
    uint8_t note_on[3] = {0x90 | channel, note_sequence[note_pos], 127};
    tud_midi_stream_write(cable_num, note_on, 3);

    // Send Note Off for previous note.
    uint8_t note_off[3] = {0x80 | channel, note_sequence[previous], 0};
    tud_midi_stream_write(cable_num, note_off, 3);

    // Increment position
    note_pos++;

    // If we are at the end of the sequence, start over.
    if (note_pos >= sizeof(note_sequence)) {
        note_pos = 0;
    }
}

#endif 

void app_main(void)
{

    // START OF USB
    ESP_LOGI(TAG, "USB initialization");


    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL, // If device_descriptor is NULL, tinyusb_driver_install() will use Kconfig
        .string_descriptor = s_str_desc,
        .external_phy = false,
        .configuration_descriptor = s_cfg_desc,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
                        TINYUSB_CDC_ACM_0,
                        CDC_EVENT_LINE_STATE_CHANGED,
                        &tinyusb_cdc_line_state_changed_callback));

    ESP_LOGI(TAG, "USB initialization DONE");

    // END OF USB

    static const char *TAG = "main";


    vTaskDelay(100);
    
    grid_esp32_swd_pico_pins_init(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, GRID_ESP32_PINS_RP_CLOCK);
    grid_esp32_swd_pico_clock_init(LEDC_TIMER_0, LEDC_CHANNEL_0);
    grid_esp32_swd_pico_program_sram(GRID_ESP32_PINS_RP_SWCLK, GRID_ESP32_PINS_RP_SWDIO, ___grid_pico_build_main_main_bin, ___grid_pico_build_main_main_bin_len);




    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();


    // GRID MODULE INITIALIZATION SEQUENCE

    grid_sys_init(&grid_sys_state);
	grid_msg_init(&grid_msg_state);

    grid_usb_midi_init();
    grid_keyboard_init(&grid_keyboard_state);

	grid_lua_init(&grid_lua_state);
	grid_lua_start_vm(&grid_lua_state);

    // ================== START: grid_module_pbf4_init() ================== //

    grid_module_pbf4_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

    grid_ui_state.ui_interaction_enabled = 1;
    // ================== FINISH: grid_module_pbf4_init() ================== //
	
	grid_port_init_all();
	//grid_d51_uart_init();
	grid_sys_set_bank(&grid_sys_state, 0);

	//grid_d51_nvm_init(&grid_d51_nvm_state, &FLASH_0);

	grid_ui_page_load(&grid_ui_state, 0); //load page 0

	//*
    while (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state))
	{
		grid_ui_bulk_pageread_next(&grid_ui_state);
	}
    



    ets_printf("GRID_SYS_TEST %d\r\n", grid_sys_get_hwcfg(&grid_sys_state));
    grid_platform_delay_ms(10);

    TaskHandle_t adc_task_hdl;


    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_adc_task,
                            "adc",
                            4096,
                            (void *)signaling_sem,
                            ADC_TASK_PRIORITY,
                            &adc_task_hdl,
                            0);

    TaskHandle_t port_task_hdl;


    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_port_task,
                            "port",
                            4096*4,
                            (void *)signaling_sem,
                            PORT_TASK_PRIORITY,
                            &port_task_hdl,
                            0);


    TaskHandle_t led_task_hdl;

    //Create the class driver task
    xTaskCreatePinnedToCore(grid_esp32_led_task,
                            "led",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);



#if CFG_TUD_MIDI

    int const tempo = 286;
    const esp_timer_create_args_t periodic_midi_args = {
        .callback = &periodic_midi_write_example_cb,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic_midi"
    };

    ESP_LOGI(TAG, "MIDI write task init");
    esp_timer_handle_t periodic_midi_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_midi_args, &periodic_midi_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_midi_timer, tempo * 1000));

    // Read recieved MIDI packets
    ESP_LOGI(TAG, "MIDI read task init");
    xTaskCreate(midi_task_read_example, "midi_task_read_example", 2 * 1024, NULL, 5, NULL);
#endif

}

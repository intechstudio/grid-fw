/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_ef44.h"
#include "../../grid_common/grid_led.h"

static const char *TAG = "esp32_adc";

//Configuration for the SPI bus
static spi_bus_config_t buscfg={
    .mosi_io_num=-1, // unused pin
    .miso_io_num=GRID_ESP32_PINS_HWCFG_DATA,
    .sclk_io_num=GRID_ESP32_PINS_HWCFG_CLOCK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
};

static spi_device_interface_config_t devcfg = {
    .command_bits=0,
    .address_bits=0,
    .dummy_bits=0,
    .clock_speed_hz=500000,
    .duty_cycle_pos=128,        //50% duty cycle
    .mode=2,
    .spics_io_num=-1,
    .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
    .queue_size=3,
    .flags = ESP_INTR_FLAG_IRAM,
    .pre_cb=my_post_setup_cb,
    .post_cb=my_post_trans_cb
};


static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t* UI_SPI_RX_BUFFER;
static uint8_t UI_SPI_RX_BUFFER_ACTUAL[14] = {0};

static volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};

static uint32_t encoder_last_real_time[16] = {0};
static uint32_t button_last_real_time[16] = {0};

static uint8_t phase_change_lock_array[16] = {0};


static void IRAM_ATTR my_post_setup_cb(spi_transaction_t *trans) {
    //printf("$\r\n");
}


static void IRAM_ATTR  my_post_trans_cb(spi_transaction_t *trans) {


    UI_SPI_RX_BUFFER = &UI_SPI_RX_BUFFER_ACTUAL[1]; // SKIP HWCFG BYTE

    uint8_t encoder_position_lookup[4] = {2, 3, 0, 1} ;
    //uint8_t encoder_position_lookup[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;


    // Buffer is only 8 bytes but we check all 16 encoders separately
    for (uint8_t j=0; j<4; j++){

        uint8_t new_value = (UI_SPI_RX_BUFFER[j/2]>>(4*(j%2)))&0x0F;
        uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];

        UI_SPI_RX_BUFFER_LAST[j] = new_value;

        
        uint8_t i = encoder_position_lookup[j];

        grid_ui_encoder_store_input(i, &encoder_last_real_time[i], &button_last_real_time[i], old_value, new_value, &phase_change_lock_array[i]);
            
    }

    // for (int i = 0; i < 4; i++) {
    //     ets_printf("%02x ", ((uint8_t*)trans->rx_buffer)[i]);
    // }
    // ets_printf("\n");


    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1);

    spi_device_queue_trans(spi, &t, 0);


}



static void adc_init(adc_oneshot_unit_handle_t* adc1_handle, adc_oneshot_unit_handle_t* adc2_handle){

    ESP_LOGI(TAG, "Init ADC");

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config1 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle, ADC_CHANNEL_1, &config1));
   
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, adc2_handle));

    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config2 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc2_handle, ADC_CHANNEL_7, &config2));

}


#define SPI_ENCODER_HOST    SPI3_HOST

void grid_esp32_module_ef44_task(void *arg)
{

    // ADC AND MULTIPLEXER INITIALIZATION        

    static uint8_t multiplexer_index = 0;
    static uint32_t last_real_time[16] = {0};

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_handle_t adc2_handle;

    adc_init(&adc1_handle, &adc2_handle);


    gpio_set_direction(GRID_ESP32_PINS_MUX_0_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_0_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_0_C, GPIO_MODE_OUTPUT);

    gpio_set_direction(GRID_ESP32_PINS_MUX_1_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_1_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_1_C, GPIO_MODE_OUTPUT);

    gpio_set_level(GRID_ESP32_PINS_MUX_0_A, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_B, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_C, 0);

    gpio_set_level(GRID_ESP32_PINS_MUX_1_A, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_B, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_C, 0);


    // SPI INITIALIZATION



    // SETUP HWCFG PINS FOR SPI MASTER TRANSACTIONS


    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_DATA, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_PULLUP_ONLY);


    gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA,  GPIO_MODE_INPUT);




    uint8_t n=0;


    ret = spi_bus_initialize(SPI_ENCODER_HOST, &buscfg, SPI_DMA_CH_AUTO);
    
    ESP_ERROR_CHECK(ret);


    ret = spi_bus_add_device(SPI_ENCODER_HOST, &devcfg, &spi);

    ESP_ERROR_CHECK(ret);

    //Configuration for the SPI slave interface

    memset(&t, 0, sizeof(t));

    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1);

    t.tx_buffer = UI_SPI_TX_BUFFER;
    t.rx_buffer = UI_SPI_RX_BUFFER_ACTUAL;
    t.length = 14 * 8;

    ret = spi_device_queue_trans(spi, &t, portMAX_DELAY);

    while (1) {

        
        //ret = spi_device_transmit(spi, &t);

        int adcresult_0 = 0;
        int adcresult_1 = 0;
        
        uint8_t adc_index_0 = multiplexer_index+2;
        uint8_t adc_index_1 = multiplexer_index;


        adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &adcresult_0);
        adc_oneshot_read(adc2_handle, ADC_CHANNEL_7, &adcresult_1);

	
        /* Update the multiplexer */

        multiplexer_index++;
        multiplexer_index%=2;


        gpio_set_level(GRID_ESP32_PINS_MUX_0_A, multiplexer_index/1%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_0_B, multiplexer_index/2%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_0_C, multiplexer_index/4%2);

        gpio_set_level(GRID_ESP32_PINS_MUX_1_A, multiplexer_index/1%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_1_B, multiplexer_index/2%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_1_C, multiplexer_index/4%2);
        
        int32_t result_resolution = 7;
        int32_t source_resolution = 12;


        grid_ui_potmeter_store_input(adc_index_0+4, &last_real_time[adc_index_0], adcresult_0, 12); // 12 bit analog values
        grid_ui_potmeter_store_input(adc_index_1+4, &last_real_time[adc_index_1], adcresult_1, 12); // 12 bit analog values

        if (multiplexer_index == 0){
            //ets_printf("%d \n", adcresult_0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_ef44.h"
#include "../../grid_common/grid_led.h"

static const char *TAG = "esp32_adc";

static uint32_t last_real_time[16] = {0};

static uint8_t multiplexer_index = 0;

static void update_mux(void){


    multiplexer_index++;
    multiplexer_index%=2;


    gpio_set_level(GRID_ESP32_PINS_MUX_0_A, multiplexer_index/1%2);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_B, multiplexer_index/2%2);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_C, multiplexer_index/4%2);

    gpio_set_level(GRID_ESP32_PINS_MUX_1_A, multiplexer_index/1%2);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_B, multiplexer_index/2%2);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_C, multiplexer_index/4%2);


}

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

#define EXAMPLE_READ_LEN                   2*SOC_ADC_DIGI_DATA_BYTES_PER_CONV

static uint8_t interrupt_count = 255;

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    interrupt_count++;

    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    //vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    if (interrupt_count%4 == 2){
        

        update_mux();

    }

    if (interrupt_count%4 == 0){

        uint32_t ret_num = 0;


        void* result = user_data;

        adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);


        //ets_printf("%d %d\r\n", ret_num, SOC_ADC_DIGI_RESULT_BYTES);

        for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {

            adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];

            uint32_t chan_num = p->type2.channel;
            uint32_t data = p->type2.data;

            // /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
            // if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
            //     ets_printf("Channel: %d, Value: %d", multiplexer_index+chan_num*2, data);
            // } else {
            //     ets_printf("Invalid data");
            // }


            int adcresult = data;
            
            uint8_t adc_index = (1-multiplexer_index)+chan_num*2;
            
            int32_t result_resolution = 7;
            int32_t source_resolution = 12;


            grid_ui_potmeter_store_input(adc_index+4, &last_real_time[adc_index], adcresult, 12); // 12 bit analog values


        }



    }


    return (mustYield == pdTRUE);
}




static void continuous_adc_init(adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 8,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    dig_cfg.pattern_num = 2;

    adc_digi_pattern_config_t adc_pattern[2] = {0};

    adc_pattern[0].atten = ADC_ATTEN_DB_11;
    adc_pattern[0].channel = ADC_CHANNEL_1 & 0x7;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;


    adc_pattern[1].atten = ADC_ATTEN_DB_11;
    adc_pattern[1].channel = ADC_CHANNEL_0 & 0x7;
    adc_pattern[1].unit = ADC_UNIT_1;
    adc_pattern[1].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}


#define SPI_ENCODER_HOST    SPI3_HOST

void grid_esp32_module_ef44_task(void *arg)
{


    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(&handle);

    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, (void*) result));
    ESP_ERROR_CHECK(adc_continuous_start(handle));


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

        

        vTaskDelay(pdMS_TO_TICKS(10));
 
    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}

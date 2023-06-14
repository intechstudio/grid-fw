/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_ef44.h"


static const char *TAG = "esp32_adc";

static uint64_t last_real_time[16] = {0};

static uint8_t DRAM_ATTR multiplexer_index = 0;


void IRAM_ATTR grid_esp32_encoder_latch_data(void){

    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1);

}


void grid_esp32_encoder_pins_init(void){

    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_DATA, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_PULLUP_ONLY);

    gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA,  GPIO_MODE_INPUT);
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

static uint64_t encoder_last_real_time[16] = {0};
static uint64_t button_last_real_time[16] = {0};

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


    grid_esp32_encoder_latch_data();

    spi_device_queue_trans(spi, &t, 0);


}



static uint8_t DRAM_ATTR interrupt_count = 255;





static TaskHandle_t DRAM_ATTR task_handle;
bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{

    BaseType_t mustYield = pdFALSE;
    gpio_ll_set_level(&GPIO, 47,1);
    vTaskNotifyGiveFromISR(task_handle, &mustYield);
    ets_delay_us(10);
    gpio_ll_set_level(&GPIO, 47,0);
    return (mustYield == pdTRUE);
}

void grid_esp32_module_ef44_task(void *arg)
{

    task_handle = xTaskGetCurrentTaskHandle();

    grid_esp32_adc_init(&grid_esp32_adc_state);
    grid_esp32_adc_register_callback(&grid_esp32_adc_state, s_conv_done_cb);
    
    grid_esp32_adc_start(&grid_esp32_adc_state);



    grid_esp32_adc_mux_pins_init();

    // SPI INITIALIZATION

    // SETUP HWCFG PINS FOR SPI MASTER TRANSACTIONS
    grid_esp32_encoder_pins_init();

    ret = spi_bus_initialize(SPI_ENCODER_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(SPI_ENCODER_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    //Configuration for the SPI slave interface

    memset(&t, 0, sizeof(t));

    grid_esp32_encoder_latch_data();

    t.tx_buffer = UI_SPI_TX_BUFFER;
    t.rx_buffer = UI_SPI_RX_BUFFER_ACTUAL;
    t.length = 14 * 8;

    ret = spi_device_queue_trans(spi, &t, portMAX_DELAY);

    
    uint8_t res[ADC_CONVERSION_FRAME_SIZE*5] = {0};

    while (1) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        uint32_t ret_num = 0;

        uint8_t channel_0_index = 0;
        uint8_t channel_1_index = 0;
        uint16_t channel_0_value = 0;
        uint16_t channel_1_value = 0;


        uint16_t channel_0_valid = true;
        uint16_t channel_1_valid = true;


        esp_err_t ret = adc_continuous_read(grid_esp32_adc_state.adc_handle, res, ADC_CONVERSION_FRAME_SIZE*5, &ret_num, 0);

        multiplexer_index++;
        multiplexer_index%=2;
        grid_esp32_adc_mux_update(multiplexer_index);

        uint32_t result_count = ret_num/SOC_ADC_DIGI_RESULT_BYTES;

        if (ret == ESP_OK) {
            


            // separated interleved results from the two channels
            channel_0_index = ((adc_digi_output_data_t*)&res)[0].type2.channel;
            channel_1_index = ((adc_digi_output_data_t*)&res)[1].type2.channel;

            uint64_t channel_0_sum = 0;
            uint64_t channel_1_sum = 0;

            uint32_t channel_0_count = 0;
            uint32_t channel_1_count = 0;

            uint16_t result_length = 0;

            // skip first couple of results
            for (int i = result_count-32; i < result_count; i++) {
                
                adc_digi_output_data_t *p = (adc_digi_output_data_t*)&res[i*SOC_ADC_DIGI_RESULT_BYTES];

                uint16_t channel = p->type2.channel;
                uint32_t data = p->type2.data;
                uint16_t adcresult = data;

                if (channel == channel_0_index){
                    
                    channel_0_sum += adcresult; 
                    channel_0_count++;

                    if ( abs(adcresult - channel_0_sum/channel_0_count) > 5){
                        //channel_0_valid = false;
                    }

                }


                if (channel == channel_1_index){
                    
                    channel_1_sum += adcresult; 
                    channel_1_count++;
                                
                    if ( abs(adcresult - channel_1_sum/channel_1_count) > 5){
                        //channel_1_valid = false;
                    }

                }


            }

            if (channel_0_count != 0){
                channel_0_value = channel_0_sum/channel_0_count;
            }
            else{
                channel_0_valid = false;
            }

            if (channel_1_count != 0){
                channel_1_value = channel_1_sum/channel_1_count;
            }
            else{
                channel_1_valid = false;
            }


        }

            
        int32_t result_resolution = 7;
        int32_t source_resolution = 12;

        if (channel_0_valid){
            grid_ui_potmeter_store_input(2*channel_0_index+4+1-multiplexer_index, &last_real_time[channel_0_index], channel_0_value, 12); // 12 bit analog values
        }
        if (channel_1_valid){
            grid_ui_potmeter_store_input(2*channel_1_index+4+1-multiplexer_index, &last_real_time[channel_1_index], channel_1_value, 12); // 12 bit analog values
        }


        // if (multiplexer_index == 0){

        //     if (channel_0_index == 0){

        //         ets_printf("%04d: %d %d %d %d \r\n", result_count,channel_0_index, channel_1_index, channel_0_value, channel_1_value);
        //     }
        //     else{

        //         ets_printf("%04d: %d %d %d %d \r\n", result_count,channel_1_index, channel_0_index, channel_1_value, channel_0_value);
        //     }
        // }


        vTaskDelay(pdMS_TO_TICKS(2));

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        adc_continuous_read(grid_esp32_adc_state.adc_handle, res, ADC_CONVERSION_FRAME_SIZE*5, &ret_num, 0);


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}

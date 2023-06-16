/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_ef44.h"


static const char *TAG = "module_ef44";

static uint64_t last_real_time[16] = {0};
static uint64_t encoder_last_real_time[16] = {0};
static uint64_t button_last_real_time[16] = {0};
static uint8_t phase_change_lock_array[16] = {0};

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t *trans) {
    //printf("$\r\n");
}

static void IRAM_ATTR  my_post_trans_cb(spi_transaction_t *trans) {

    

    uint8_t* spi_rx_buffer = &trans->rx_buffer[1]; // SKIP HWCFG BYTE

    uint8_t encoder_position_lookup[4] = {2, 3, 0, 1} ;
    //uint8_t encoder_position_lookup[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;


    // Buffer is only 8 bytes but we check all 16 encoders separately
    for (uint8_t j=0; j<4; j++){

        uint8_t new_value = (spi_rx_buffer[j/2]>>(4*(j%2)))&0x0F;
        uint8_t old_value = grid_esp32_encoder_state.rx_buffer_previous[j];

        grid_esp32_encoder_state.rx_buffer_previous[j] = new_value;

        
        uint8_t i = encoder_position_lookup[j];

        grid_ui_encoder_store_input(i, &encoder_last_real_time[i], &button_last_real_time[i], old_value, new_value, &phase_change_lock_array[i]);
            
    }

    // for (int i = 0; i < 4; i++) {
    //     ets_printf("%02x ", ((uint8_t*)trans->rx_buffer)[i]);
    // }
    // ets_printf("\n");


    grid_esp32_encoder_spi_start_transfer(&grid_esp32_encoder_state);


}

static TaskHandle_t DRAM_ATTR task_handle;
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{

    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(task_handle, &mustYield);
    return (mustYield == pdTRUE);
}

void grid_esp32_module_ef44_task(void *arg)
{

    task_handle = xTaskGetCurrentTaskHandle();

    grid_esp32_adc_init(&grid_esp32_adc_state);
    grid_esp32_adc_mux_pins_init(&grid_esp32_adc_state);
    grid_esp32_adc_register_callback(&grid_esp32_adc_state, s_conv_done_cb);
    grid_esp32_adc_start(&grid_esp32_adc_state);

    grid_esp32_encoder_init(&grid_esp32_encoder_state, my_post_setup_cb, my_post_trans_cb);
    grid_esp32_encoder_spi_start_transfer(&grid_esp32_encoder_state);

    
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


        grid_esp32_adc_mux_increment(&grid_esp32_adc_state, 2);
        grid_esp32_adc_mux_update(&grid_esp32_adc_state);

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

            
        uint8_t multiplexer_index = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);

        if (channel_0_valid){
            grid_ui_potmeter_store_input(2*channel_0_index+4+1-multiplexer_index, &last_real_time[channel_0_index], channel_0_value, grid_platform_get_adc_bit_depth()); // 12 bit analog values
        }
        if (channel_1_valid){
            grid_ui_potmeter_store_input(2*channel_1_index+4+1-multiplexer_index, &last_real_time[channel_1_index], channel_1_value, grid_platform_get_adc_bit_depth()); // 12 bit analog values
        }


        vTaskDelay(pdMS_TO_TICKS(2));

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        adc_continuous_read(grid_esp32_adc_state.adc_handle, res, ADC_CONVERSION_FRAME_SIZE*5, &ret_num, 0);


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}

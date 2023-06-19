/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_adc.h"

struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;



bool IRAM_ATTR grid_esp32_adc_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{


    struct grid_esp32_adc_model* adc = (struct grid_esp32_adc_model*) user_data;

    BaseType_t mustYield = pdFALSE;

    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;


    if (xSemaphoreTakeFromISR(adc->nvm_semaphore, NULL) == pdTRUE){

        adc->adc_interrupt_state++;

        if (adc->adc_interrupt_state%3 == 0){ // update the multiplexer

            // update multiplexer
            grid_esp32_adc_mux_increment(&grid_esp32_adc_state);
            grid_esp32_adc_mux_update(&grid_esp32_adc_state);

        }
        else if(adc->adc_interrupt_state%3 == 1){ // purge previous results
                    
            uint32_t ret_num = 0;

            do{
                // purge buffer
                ret = adc_continuous_read(handle, adc->adc_result_buffer, ADC_CONVERSION_FRAME_SIZE, &ret_num, 0);

            }while(ret_num);

        }
        else{ // process valid results

            uint32_t ret_num = 0;
            adc->adc_interrupt_state = -1;

            ret = adc_continuous_read(handle, adc->adc_result_buffer, ADC_CONVERSION_FRAME_SIZE, &ret_num, 0);

            uint32_t result_count = ret_num/SOC_ADC_DIGI_RESULT_BYTES;

            if (ret == ESP_OK) {
 

                struct grid_esp32_adc_preprocessor channel_A = {
                    .average = 0,
                    .channel = 0,
                    .count = 0,
                    .sum = 0
                } ;

                struct grid_esp32_adc_preprocessor channel_B = {
                    .average = 0,
                    .channel = 0,
                    .count = 0,
                    .sum = 0
                } ;



                // skip first couple of results
                for (int i = result_count-32; i < result_count; i++) {
                    
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&adc->adc_result_buffer[i*SOC_ADC_DIGI_RESULT_BYTES];
                    struct grid_esp32_adc_preprocessor* channel = ((i%2==0)?&channel_A:&channel_B);

                    channel->channel = p->type2.channel;
                    channel->sum+=(uint16_t) p->type2.data;
                    channel->count++;
                    channel->average = channel->sum/channel->count;

                }

                channel_A.average = channel_A.sum/channel_A.count;
                channel_B.average = channel_B.sum/channel_B.count;
            

                // store results
                for (int i = 0; i < 2; i++) {

                    struct grid_esp32_adc_preprocessor* channel = ((i%2==0)?&channel_A:&channel_B);
                    
                    if (channel->count){
                        struct grid_esp32_adc_result result;
                        result.channel = channel->channel;
                        result.mux_state = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);;
                        result.value = channel->sum/channel->count;
                        xRingbufferSendFromISR(adc->ringbuffer_handle , &result, sizeof(struct grid_esp32_adc_result), NULL);
                    }
                    else{
                        ets_printf("$%d\r\n", i);
                    }
                    
                }

            }
                

        }


        



        
        xSemaphoreGiveFromISR(adc->nvm_semaphore, NULL);
    }



    return (mustYield == pdTRUE);

    
}



void continuous_adc_init(adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = ADC_BUFFER_SIZE,
        .conv_frame_size = ADC_CONVERSION_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_HIGH/2,
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



void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow){

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

    adc->mux_overflow = mux_overflow;
}


void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc){
    adc->mux_index++;
    adc->mux_index%=adc->mux_overflow;
}

void IRAM_ATTR grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc){

    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_A, adc->mux_index/1%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_B, adc->mux_index/2%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_C, adc->mux_index/4%2);

    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_A, adc->mux_index/1%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_B, adc->mux_index/2%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_C, adc->mux_index/4%2);

}

uint8_t IRAM_ATTR grid_esp32_adc_mux_get_index(struct grid_esp32_adc_model* adc){
    return adc->mux_index;
}

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, SemaphoreHandle_t nvm_semaphore){

    adc->nvm_semaphore = nvm_semaphore;

    adc->adc_handle = NULL;
    adc->adc_interrupt_state = 0;

    adc->adc_result_buffer = (uint8_t*) heap_caps_malloc(ADC_CONVERSION_FRAME_SIZE * sizeof(uint8_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    adc->buffer_struct = (StaticRingbuffer_t *)heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    adc->buffer_storage = (struct grid_esp32_adc_result *)heap_caps_malloc(sizeof(struct grid_esp32_adc_result)*BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    adc->ringbuffer_handle = xRingbufferCreateStatic(BUFFER_SIZE, BUFFER_TYPE, adc->buffer_storage, adc->buffer_struct);


    continuous_adc_init(&adc->adc_handle);


    

    adc->mux_index = 0;

}

void grid_esp32_adc_register_callback(struct grid_esp32_adc_model* adc, void (*callback)(adc_continuous_handle_t, const adc_continuous_evt_data_t*, void*)){
   
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = callback,
    };

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc->adc_handle, &cbs, (void*)adc));

}


void grid_esp32_adc_start(struct grid_esp32_adc_model* adc){


    ESP_ERROR_CHECK(adc_continuous_start(adc->adc_handle));

}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc){


    ESP_ERROR_CHECK(adc_continuous_stop(adc->adc_handle));

}


/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_encoder.h"



struct grid_esp32_encoder_model DRAM_ATTR grid_esp32_encoder_state;


void grid_esp32_encoder_pins_init(void){

    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_DATA, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_PULLUP_ONLY);

    gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA,  GPIO_MODE_INPUT);

}

void grid_esp32_encoder_spi_init(struct grid_esp32_encoder_model* encoder, void (*post_setup_cb)(spi_transaction_t*), void (*post_trans_cb)(spi_transaction_t*)){

    //Configuration for the SPI bus
    static spi_bus_config_t buscfg={
        .mosi_io_num=-1, // unused pin
        .miso_io_num=GRID_ESP32_PINS_HWCFG_DATA,
        .sclk_io_num=GRID_ESP32_PINS_HWCFG_CLOCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    static spi_device_interface_config_t devcfg = {
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=1000000,    //was 500k
        .duty_cycle_pos=128,        //50% duty cycle
        .mode=2,
        .spics_io_num=-1,
        .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=3,
        .flags = ESP_INTR_FLAG_IRAM
    };

    devcfg.pre_cb = post_setup_cb;
    devcfg.pre_cb = post_trans_cb;


    esp_err_t ret = spi_bus_initialize(encoder->spi_host, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(encoder->spi_host, &devcfg, &encoder->spi_device_handle);
    ESP_ERROR_CHECK(ret);

    //Configuration for the SPI slave interface

}


void IRAM_ATTR grid_esp32_encoder_latch_data(void){

    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_HWCFG_SHIFT, 0);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_HWCFG_SHIFT, 1);

}

void grid_esp32_encoder_init(struct grid_esp32_encoder_model* encoder, void (*post_setup_cb)(spi_transaction_t*), void (*post_trans_cb)(spi_transaction_t*)){

    encoder->spi_host = SPI_ENCODER_HOST;
    encoder->spi_device_handle = NULL;

    encoder->rx_buffer_previous =   (uint8_t*) malloc(GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));
    encoder->rx_buffer =            (uint8_t*) malloc(GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));
    encoder->tx_buffer =            (uint8_t*) malloc(GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));

    memset(encoder->rx_buffer_previous, 0, GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));
    memset(encoder->rx_buffer,          0, GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));
    memset(encoder->tx_buffer,          0, GRID_ESP32_ENCODER_BUFFER_SIZE * sizeof(uint8_t));

    
    
    memset(&encoder->transaction, 0, sizeof(encoder->transaction));

    encoder->transaction.tx_buffer = encoder->tx_buffer;
    encoder->transaction.rx_buffer = encoder->rx_buffer;
    encoder->transaction.length = GRID_ESP32_ENCODER_BUFFER_SIZE * 8;

    grid_esp32_encoder_pins_init();

    grid_esp32_encoder_spi_init(encoder, post_setup_cb, post_trans_cb);

    encoder->buffer_struct = (StaticRingbuffer_t *)heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    encoder->buffer_storage = (struct grid_esp32_encoder_result *)heap_caps_malloc(sizeof(struct grid_esp32_encoder_result)*ENCODER_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    encoder->ringbuffer_handle = xRingbufferCreateStatic(ENCODER_BUFFER_SIZE, ENCODER_BUFFER_TYPE, encoder->buffer_storage, encoder->buffer_struct);


    return;

}


void IRAM_ATTR grid_esp32_encoder_spi_start_transfer(struct grid_esp32_encoder_model* encoder){

    grid_esp32_encoder_latch_data();
    spi_device_queue_trans(encoder->spi_device_handle, &encoder->transaction, 0);

}
#pragma once


#include "hardware/irq.h"



void grid_pico_spi_init(void);
void spi_start_transfer(uint tx_channel, uint rx_channel, uint8_t* tx_buffer, uint8_t* rx_buffer, irq_handler_t callback);
void grid_pico_spi_dma_xfer_complete_cb(void);

extern volatile uint8_t spi_dma_done;
extern uint grid_pico_spi_dma_tx_channel;
extern uint grid_pico_spi_dma_rx_channel;
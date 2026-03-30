#pragma once

#include "hardware/irq.h"

int grid_pico_spi_isready(void);
int grid_pico_spi_is_rx_data_available(void);
void grid_pico_spi_set_rx_data_available_flag(void);
void grid_pico_spi_clear_rx_data_available_flag(void);

void grid_pico_spi_init(void);
void grid_pico_spi_transfer(uint8_t* tx_buffer, uint8_t* rx_buffer);
void grid_pico_spi_dma_xfer_complete_cb(void);

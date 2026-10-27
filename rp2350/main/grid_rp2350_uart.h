#ifndef GRID_RP2350_UART_H
#define GRID_RP2350_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_protocol.h"
#include "grid_swsr.h"

enum { GRID_RP2350_UART_DIR_COUNT = 4 };

extern struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
extern uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

void grid_rp2350_uart_init(void);

void grid_rp2350_uart_port_stop_dma(uint8_t dir);

void grid_rp2350_uart_port_reset_dma(uint8_t dir);

void grid_rp2350_uart_tx_start(uint8_t dir, uint32_t size);

bool grid_rp2350_uart_tx_busy(uint8_t dir);

#endif /* GRID_RP2350_UART_H */

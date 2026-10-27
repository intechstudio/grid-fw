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

// Set to 1 (or pass -DGRID_RP2350_UART_DEBUG=1) to count RX framing errors and
// print per-direction PIO/DMA state over RTT once a second.
#ifndef GRID_RP2350_UART_DEBUG
#define GRID_RP2350_UART_DEBUG 0
#endif

#if GRID_RP2350_UART_DEBUG
void grid_rp2350_uart_debug_poll(void);
#else
static inline void grid_rp2350_uart_debug_poll(void) {}
#endif

// Convert received PIO frames into uwsr bytes; call before reading the uwsr.
void grid_rp2350_uart_rx_poll(uint8_t dir);

void grid_rp2350_uart_port_stop_dma(uint8_t dir);

void grid_rp2350_uart_port_reset_dma(uint8_t dir);

void grid_rp2350_uart_tx_start(uint8_t dir, uint32_t size);

bool grid_rp2350_uart_tx_busy(uint8_t dir);

#endif /* GRID_RP2350_UART_H */

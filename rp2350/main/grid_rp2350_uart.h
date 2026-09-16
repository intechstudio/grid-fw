#ifndef GRID_RP2350_UART_H
#define GRID_RP2350_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_protocol.h"
#include "grid_swsr.h"

// 4-directional USART daisy-chain over PIO (no free real UART peripheral) --
// same DMA/interrupt shape as D51's real-UART-per-direction model.

enum { GRID_RP2350_UART_DIR_COUNT = 4 };

extern struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
extern uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

void grid_rp2350_uart_init(void);

// Stops dir's RX DMA without rewinding/rearming, so a caller (grid_port_recv_uwsr's
// overflow handling) can touch uwsr state before rearming via reset_dma below.
void grid_rp2350_uart_port_stop_dma(uint8_t dir);

// Stops (see above) then rewinds+rearms dir's RX DMA from the start of its
// uwsr buffer. Called from the completion IRQ and grid_platform_reset_grid_transmitter.
void grid_rp2350_uart_port_reset_dma(uint8_t dir);

// Kicks off a one-shot DMA transfer of the first `size` staged bytes in
// grid_rp2350_uart_tx_buf[dir]. Caller must confirm !grid_rp2350_uart_tx_busy(dir) first.
void grid_rp2350_uart_tx_start(uint8_t dir, uint32_t size);

// True while dir's TX DMA channel is still draining -- reads
// dma_channel_is_busy() directly rather than tracking a separate flag.
bool grid_rp2350_uart_tx_busy(uint8_t dir);

#endif /* GRID_RP2350_UART_H */

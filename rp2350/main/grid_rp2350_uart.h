#ifndef GRID_RP2350_UART_H
#define GRID_RP2350_UART_H

#include <stdint.h>

#include "grid_msg.h"
#include "grid_port.h"
#include "grid_swsr.h"

// 4-directional (N/E/S/W) USART daisy-chain transport. Mirrors
// d51n20a/grid/d51/grid_d51_uart.h's real-DMA-per-direction model, not the
// RP2040 coprocessor's PIO "bucket" pool (rp2040/src/pico_pool.c) -- RP2350
// has no free hardware UART peripheral (UART0=stdio console, UART1=WS2812
// LED), so both TX and RX run over PIO state machines instead of a real PL011
// peripheral, but the DMA/interrupt architecture around them is otherwise the
// same shape as D51's: a fixed grid_uwsr_t per direction fed directly by DMA,
// completion-interrupt driven, no intermediate pooling/copy layer.

enum { GRID_RP2350_UART_DIR_COUNT = 4 };

extern struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
extern uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

void grid_rp2350_uart_init(void);

// Stops direction dir's RX DMA transfer without rewinding/rearming it, so a
// caller can safely touch its uwsr buffer's software state before the
// rearm -- see grid_rp2350_uart_port_recv's overflow handling.
void grid_rp2350_uart_port_stop_dma(uint8_t dir);

// Stops (see above) then rewinds+rearms direction dir's RX DMA transfer from
// the start of its uwsr buffer. Called from the completion IRQ and from
// grid_platform_reset_grid_transmitter (grid_rp2350_platform.c, the USART
// disconnect path) -- neither of those touches uwsr's software state
// concurrently, so the single combined call is safe there.
void grid_rp2350_uart_port_reset_dma(uint8_t dir);

// Kicks off a one-shot DMA transfer of the first `size` bytes already staged
// in grid_rp2350_uart_tx_buf[dir] into that direction's TX state machine.
// Caller (grid_platform_send_frame) must have linearized the message into
// the staging buffer first and confirmed !grid_rp2350_uart_tx_busy(dir).
void grid_rp2350_uart_tx_start(uint8_t dir, uint32_t size);

// True while direction dir's TX DMA channel is still draining
// grid_rp2350_uart_tx_buf[dir] -- reads dma_channel_is_busy() directly
// rather than tracking a separately-maintained ready flag.
bool grid_rp2350_uart_tx_busy(uint8_t dir);

// Drains and parses one complete message (if any) out of uwsr for port,
// de-duplicating broadcasts via fpb. Ports d51n20a/grid_d51n20a.c's
// grid_d51_port_recv_uwsr verbatim -- built entirely from shared
// common/src/c functions, no platform-specific logic.
void grid_rp2350_uart_port_recv(struct grid_port* port, struct grid_uwsr_t* uwsr, struct grid_fingerprint_buf* fpb);

#endif /* GRID_RP2350_UART_H */

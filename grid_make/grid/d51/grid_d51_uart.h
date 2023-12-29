/*
 * grid_d51_uart.h
 *
 * Created: 6/3/2020 5:02:04 PM
 *  Author: WPC-User
 */

#ifndef GRID_D51_UART_H_
#define GRID_D51_UART_H_

#include "grid_d51_module.h"

//====================== DMA CONFIGURATION FOR GRID USART RX C
//===================================//

#define DMA_NORTH_RX_CHANNEL 0
#define DMA_EAST_RX_CHANNEL 1
#define DMA_SOUTH_RX_CHANNEL 2
#define DMA_WEST_RX_CHANNEL 3

void grid_d51_uart_port_reset_dma(uint8_t dma_channel);

void grid_d51_uart_init();

//=========================== SYS CB ============================//

struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;

static void
err_cb_USART_GRID_N(const struct usart_async_descriptor *const descr);
static void
err_cb_USART_GRID_E(const struct usart_async_descriptor *const descr);
static void
err_cb_USART_GRID_S(const struct usart_async_descriptor *const descr);
static void
err_cb_USART_GRID_W(const struct usart_async_descriptor *const descr);
static void err_cb_USART_GRID(struct grid_port *const por);

static void
tx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr);
static void
tx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr);
static void
tx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr);
static void
tx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr);
static void tx_cb_USART_GRID(struct grid_port *const por);

static void
rx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr);
static void
rx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr);
static void
rx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr);
static void
rx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr);
static void rx_cb_USART_GRID(struct grid_port *const por);

static void dma_transfer_complete_n_cb(struct _dma_resource *resource);
static void dma_transfer_complete_e_cb(struct _dma_resource *resource);
static void dma_transfer_complete_s_cb(struct _dma_resource *resource);
static void dma_transfer_complete_w_cb(struct _dma_resource *resource);
static void dma_transfer_complete(struct grid_port *por);

void grid_d51_uart_dma_rx_init_one(struct usart_async_descriptor *usart,
                                   uint8_t channel, uint8_t *buffer,
                                   uint32_t length, void *transfer_done_cb());
static void grid_d51_uart_dma_rx_init();

#endif /* GRID_D51_UART_H_ */

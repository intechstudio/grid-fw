#include "grid_rp2350_uart.h"

#include <assert.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#include "grid_transport.h"

// Real UARTs already claimed (UART0=stdio, UART1=WS2812 LED) -- daisy-chain
// runs on PIO instead: one TX block and one RX block, each shared by 4 SMs.
#define GRID_RP2350_UART_TX_PIO pio0
#define GRID_RP2350_UART_RX_PIO pio1

// GPIO9 (WEST RX) doubles as stdio console RX (harmless, nothing reads stdin).
static const uint8_t grid_rp2350_uart_tx_pin[GRID_RP2350_UART_DIR_COUNT] = {22, 20, 10, 8};
static const uint8_t grid_rp2350_uart_rx_pin[GRID_RP2350_UART_DIR_COUNT] = {7, 21, 19, 9};

struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

static uint grid_rp2350_uart_tx_sm[GRID_RP2350_UART_DIR_COUNT];
static uint grid_rp2350_uart_rx_sm[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_tx_dma_chan[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_rx_dma_chan[GRID_RP2350_UART_DIR_COUNT];

// Shift-right without autopush leaves the byte left-justified in the FIFO
// word's top byte -- same "+3" trick as uart_rx_program_getc, via DMA here.
static inline volatile uint8_t* grid_rp2350_uart_rxf_byte(uint sm) { return (volatile uint8_t*)&GRID_RP2350_UART_RX_PIO->rxf[sm] + 3; }

void __not_in_flash_func(grid_rp2350_uart_port_stop_dma)(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);
  int chan = grid_rp2350_uart_rx_dma_chan[dir];

  // Disable DMA_IRQ_2 completely, as aborting a channel that is about to
  // complete is inherently race-prone if it has an IRQ handler registered.
  bool irq2_was_enabled = irq_is_enabled(DMA_IRQ_2);
  irq_set_enabled(DMA_IRQ_2, false);

  // Due to errata RP2350-E5, the enable bit of the aborted channel and any
  // channel chained to it must be cleared before aborting to ensure that the
  // channel isn't susceptible to re-triggering.
  uint32_t values = chan << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB | 0u << DMA_CH0_CTRL_TRIG_EN_LSB;
  uint32_t write_mask = DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS | DMA_CH0_CTRL_TRIG_EN_BITS;
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, values, write_mask);

  dma_channel_abort(chan);

  // Clear channel interrupt by writing a bitmask
  dma_hw->ints2 = 1u << chan;

  // Restore DMA_IRQ_2 to its previous state
  irq_set_enabled(DMA_IRQ_2, irq2_was_enabled);
}

void __not_in_flash_func(grid_rp2350_uart_port_reset_dma)(uint8_t dir) {

  grid_rp2350_uart_port_stop_dma(dir);

  assert(dir < GRID_RP2350_UART_DIR_COUNT);
  int chan = grid_rp2350_uart_rx_dma_chan[dir];

  // Re-enable DMA channel after stopping so it responds to the initial trigger
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS, DMA_CH0_CTRL_TRIG_EN_BITS);

  dma_channel_set_write_addr(chan, grid_rp2350_uart_uwsr[dir].data, false);
  dma_channel_set_trans_count(chan, grid_rp2350_uart_uwsr[dir].capacity, true);
}

static void __not_in_flash_func(grid_rp2350_uart_rx_dma_irq)(void) {

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

    int chan = grid_rp2350_uart_rx_dma_chan[dir];

    // Check the INTS2 register to see if this channel is asserting IRQ 2
    if (dma_hw->ints2 & (1u << chan)) {

      // Clear channel interrupt by writing a bitmask
      dma_hw->ints2 = 1u << chan;

      grid_rp2350_uart_port_reset_dma(dir);
    }
  }
}

void grid_rp2350_uart_tx_start(uint8_t dir, uint32_t size) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);
  assert(size > 0 && size <= GRID_PARAMETER_SPI_TRANSACTION_length);

  int chan = grid_rp2350_uart_tx_dma_chan[dir];
  dma_channel_set_read_addr(chan, grid_rp2350_uart_tx_buf[dir], false);
  dma_channel_set_trans_count(chan, size, true);
}

bool grid_rp2350_uart_tx_busy(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  return dma_channel_is_busy(grid_rp2350_uart_tx_dma_chan[dir]);
}

void grid_rp2350_uart_init(void) {

  uint tx_offset = pio_add_program(GRID_RP2350_UART_TX_PIO, &uart_tx_program);
  uint rx_offset = pio_add_program(GRID_RP2350_UART_RX_PIO, &uart_rx_program);

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

    assert(grid_uwsr_malloc(&grid_rp2350_uart_uwsr[dir], GRID_PORT_SWSR_SIZE, '\n') == 0);

    grid_rp2350_uart_tx_sm[dir] = pio_claim_unused_sm(GRID_RP2350_UART_TX_PIO, true);
    uart_tx_program_init(GRID_RP2350_UART_TX_PIO, grid_rp2350_uart_tx_sm[dir], tx_offset, grid_rp2350_uart_tx_pin[dir], GRID_PARAMETER_UART_baudrate);

    grid_rp2350_uart_rx_sm[dir] = pio_claim_unused_sm(GRID_RP2350_UART_RX_PIO, true);
    uart_rx_program_init(GRID_RP2350_UART_RX_PIO, grid_rp2350_uart_rx_sm[dir], rx_offset, grid_rp2350_uart_rx_pin[dir], GRID_PARAMETER_UART_baudrate);

    int tx_chan = dma_claim_unused_channel(true);
    grid_rp2350_uart_tx_dma_chan[dir] = tx_chan;
    dma_channel_config tx_cfg = dma_channel_get_default_config(tx_chan);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    uint tx_dreq = pio_get_dreq(GRID_RP2350_UART_TX_PIO, grid_rp2350_uart_tx_sm[dir], true);
    channel_config_set_dreq(&tx_cfg, tx_dreq);
    dma_channel_configure(tx_chan, &tx_cfg, &GRID_RP2350_UART_TX_PIO->txf[grid_rp2350_uart_tx_sm[dir]], grid_rp2350_uart_tx_buf[dir], 0, false);

    int rx_chan = dma_claim_unused_channel(true);
    grid_rp2350_uart_rx_dma_chan[dir] = rx_chan;
    dma_channel_config rx_cfg = dma_channel_get_default_config(rx_chan);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, true);
    uint rx_dreq = pio_get_dreq(GRID_RP2350_UART_RX_PIO, grid_rp2350_uart_rx_sm[dir], false);
    channel_config_set_dreq(&rx_cfg, rx_dreq);
    dma_channel_configure(rx_chan, &rx_cfg, grid_rp2350_uart_uwsr[dir].data, grid_rp2350_uart_rxf_byte(grid_rp2350_uart_rx_sm[dir]), grid_rp2350_uart_uwsr[dir].capacity, true);
    dma_irqn_set_channel_enabled(2, rx_chan, true);
  }

  // Above PICO_DEFAULT_IRQ_PRIORITY so that other IRQs can be masked separately
  irq_set_priority(DMA_IRQ_2, PICO_HIGHEST_IRQ_PRIORITY);

  irq_set_exclusive_handler(DMA_IRQ_2, grid_rp2350_uart_rx_dma_irq);
  irq_set_enabled(DMA_IRQ_2, true);
}

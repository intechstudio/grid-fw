#include "grid_rp2350_uart.h"

#include <assert.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_transport.h"

// Both of RP2350's real PL011 UARTs are already claimed (UART0=stdio console,
// UART1=WS2812 LED, see grid_rp2350_led.c) -- all 4 daisy-chain directions run
// on PIO instead. One block is loaded once with the TX program and shares it
// across 4 SMs (one per direction), a second block does the same for RX,
// mirroring the split rp2040/src/grid_pico_pio.c already uses to avoid
// reloading a program per SM. The third PIO block (12 - 8 = 4 SMs) stays free.
#define GRID_RP2350_UART_TX_PIO pio0
#define GRID_RP2350_UART_RX_PIO pio1

// Direction order matches enum grid_port_dir (grid_port.h): NORTH, EAST,
// SOUTH, WEST. GPIO9 (WEST RX) doubles as PICO_DEFAULT_UART_RX_PIN (stdio
// console RX, see rp2350/main/CMakeLists.txt) -- harmless since nothing reads
// stdin, but worth knowing if the console ever needs real RX in the future.
static const uint8_t grid_rp2350_uart_tx_pin[GRID_RP2350_UART_DIR_COUNT] = {22, 20, 10, 8};
static const uint8_t grid_rp2350_uart_rx_pin[GRID_RP2350_UART_DIR_COUNT] = {7, 21, 19, 9};

struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

static uint grid_rp2350_uart_tx_sm[GRID_RP2350_UART_DIR_COUNT];
static uint grid_rp2350_uart_rx_sm[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_tx_dma_chan[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_rx_dma_chan[GRID_RP2350_UART_DIR_COUNT];

// The uart_rx program's ISR shifts right without autopush (32-bit threshold),
// so after 8 sampled bits the byte sits left-justified in the FIFO word's
// uppermost byte -- same "+3" trick as the .pio file's own
// uart_rx_program_getc, just driven by DMA instead of a polled CPU read.
static inline volatile uint8_t* grid_rp2350_uart_rxf_byte(uint sm) { return (volatile uint8_t*)&GRID_RP2350_UART_RX_PIO->rxf[sm] + 3; }

// Stops direction dir's RX DMA transfer without rewinding/rearming it --
// split out from grid_rp2350_uart_port_reset_dma so a caller can quiesce the
// channel, safely touch its uwsr buffer's software state, and only then
// rearm (see grid_rp2350_uart_port_recv's overflow handling, which needs
// exactly that ordering to avoid a stale-DMA-offset race).
void grid_rp2350_uart_port_stop_dma(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  int chan = grid_rp2350_uart_rx_dma_chan[dir];

  // Mask the whole DMA_IRQ_2 line (not just this channel) for the critical
  // section below. This function runs both from mainline (USART disconnect,
  // RX-overflow recovery) and from the completion ISR itself
  // (grid_rp2350_uart_rx_dma_irq) -- without this, a genuine completion for
  // this exact channel landing between entry and the errata-mandated
  // EN-clear just below could preempt a mainline caller mid-sequence and run
  // a second, overlapping stop+rearm for the same channel, discarding
  // whatever landed in the gap between the two rearms. Calling this from
  // inside the ISR is harmless: NVIC won't re-enter the same line at the
  // same priority regardless, so it's a no-op mask/restore there. This
  // coarser whole-line mask also covers what the errata fix itself needs
  // (suppressing the spurious IRQ dma_channel_abort can raise), so the
  // narrower per-channel dma_irqn_set_channel_enabled toggle this replaced
  // is no longer needed.
  bool irq2_was_enabled = irq_is_enabled(DMA_IRQ_2);
  irq_set_enabled(DMA_IRQ_2, false);

  // RP2350 errata RP2350-E5 (see hardware/dma.h's dma_channel_abort doc): the
  // channel's enable bit must be cleared before the abort, or an in-flight
  // channel can silently re-trigger instead of actually stopping. Mirrors
  // the pico-sdk's own dma_channel_cleanup() (hardware_dma/dma.c): clear
  // CHAIN_TO/EN, abort, then clear any pending status it left behind.
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, (chan << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (0u << DMA_CH0_CTRL_TRIG_EN_LSB), DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS | DMA_CH0_CTRL_TRIG_EN_BITS);
  dma_channel_abort(chan);
  dma_hw->ints2 = 1u << chan;

  irq_set_enabled(DMA_IRQ_2, irq2_was_enabled);
}

void grid_rp2350_uart_port_reset_dma(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  // Mirrors grid_d51_uart_port_reset_dma's unconditional disable+retrigger --
  // called both from the completion IRQ (transfer already finished) and from
  // an externally forced reset (transfer still mid-flight), so the hard abort
  // is needed for the latter case and harmless for the former.
  grid_rp2350_uart_port_stop_dma(dir);

  int chan = grid_rp2350_uart_rx_dma_chan[dir];

  // stop_dma leaves the channel disabled (EN=0) as its contract, for callers
  // that need it to stay stopped (grid_rp2350_uart_port_recv's overflow
  // handling). But per DMA_CH0_CTRL_TRIG_EN's own register doc: "When 0, the
  // channel will ignore triggers" -- so EN must be set back to 1 here before
  // the retrigger below, or the trigger write is silently ignored and the
  // channel never actually restarts.
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS, DMA_CH0_CTRL_TRIG_EN_BITS);

  dma_channel_set_write_addr(chan, grid_rp2350_uart_uwsr[dir].data, false);
  dma_channel_set_trans_count(chan, grid_rp2350_uart_uwsr[dir].capacity, true);
}

static void grid_rp2350_uart_rx_dma_irq(void) {

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

    int chan = grid_rp2350_uart_rx_dma_chan[dir];

    if (dma_hw->ints2 & (1u << chan)) {

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

// dma_channel_is_busy() already exposes TX-in-flight state directly from
// hardware -- no need for a separately-maintained ready flag/ISR pair
// tracking the same bit as a second source of truth.
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

    // TX DMA: fed on demand by grid_rp2350_uart_tx_start, one byte per FIFO
    // slot (narrow 8-bit write lands the byte in the OSR's low byte, which is
    // what the TX program's right-shift-out expects).
    int tx_chan = dma_claim_unused_channel(true);
    grid_rp2350_uart_tx_dma_chan[dir] = tx_chan;
    dma_channel_config tx_cfg = dma_channel_get_default_config(tx_chan);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    channel_config_set_dreq(&tx_cfg, pio_get_dreq(GRID_RP2350_UART_TX_PIO, grid_rp2350_uart_tx_sm[dir], true));
    dma_channel_configure(tx_chan, &tx_cfg, &GRID_RP2350_UART_TX_PIO->txf[grid_rp2350_uart_tx_sm[dir]], grid_rp2350_uart_tx_buf[dir], 0, false);

    // RX DMA: DREQ-paced off the RX FIFO, fixed-length transfer restarted on
    // completion by grid_rp2350_uart_rx_dma_irq -- same shape as
    // grid_rp2350_adc.c's DREQ-paced, completion-IRQ-rearmed sweep.
    int rx_chan = dma_claim_unused_channel(true);
    grid_rp2350_uart_rx_dma_chan[dir] = rx_chan;
    dma_channel_config rx_cfg = dma_channel_get_default_config(rx_chan);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, true);
    channel_config_set_dreq(&rx_cfg, pio_get_dreq(GRID_RP2350_UART_RX_PIO, grid_rp2350_uart_rx_sm[dir], false));
    dma_channel_configure(rx_chan, &rx_cfg, grid_rp2350_uart_uwsr[dir].data, grid_rp2350_uart_rxf_byte(grid_rp2350_uart_rx_sm[dir]), grid_rp2350_uart_uwsr[dir].capacity, true);
    dma_irqn_set_channel_enabled(2, rx_chan, true);
  }

  // DMA_IRQ_0 (ADC) and DMA_IRQ_1 (encoder) are already claimed exclusively
  // (grid_rp2350_adc.c, grid_rp2350_encoder.c) -- this ISR only restarts DMA,
  // never touching ui->element_list, so unlike ADC/encoder it does NOT need
  // gating around bulk UI operations. TX has no completion IRQ of its own
  // (DMA_IRQ_3 unused) -- grid_rp2350_uart_tx_busy() reads dma_channel_is_busy()
  // directly instead.
  irq_set_exclusive_handler(DMA_IRQ_2, grid_rp2350_uart_rx_dma_irq);
  irq_set_enabled(DMA_IRQ_2, true);
}

// Ported from d51n20a/grid_d51n20a.c's grid_d51_port_recv_uwsr, built
// entirely from shared common/src/c functions -- no platform-specific logic,
// except the overflow branch below, which deliberately differs from D51's
// literal source line.
void grid_rp2350_uart_port_recv(struct grid_port* port, struct grid_uwsr_t* uwsr, struct grid_fingerprint_buf* fpb) {

  if (grid_uwsr_overflow(uwsr)) {

    // Stop DMA *before* touching uwsr's software state, not after (as D51's
    // grid_d51_port_recv_uwsr does via a single grid_platform_reset_grid_transmitter
    // call following grid_uwsr_init): otherwise the DMA can still be writing
    // at its old hardware offset while grid_uwsr_init has already zeroed the
    // buffer and reset the read/seek indices to 0, so bytes landing at that
    // stale offset are silently discarded once the DMA is later rewound.
    // Calling the RP2350-specific stop/reset pair directly (rather than
    // through the generic grid_platform_reset_grid_transmitter hook D51 uses
    // here) also sidesteps that hook's direction-encoding ambiguity, since
    // port->dir is already the plain enum this file's own functions expect.
    grid_rp2350_uart_port_stop_dma(port->dir);
    grid_uwsr_init(uwsr, uwsr->reject);
    grid_rp2350_uart_port_reset_dma(port->dir);
  }

  struct grid_msg msg;

  if (!grid_msg_from_uwsr(&msg, uwsr)) {
    return;
  }

  if (grid_frame_verify((uint8_t*)msg.data, msg.length) != 0) {
    return;
  }

  grid_str_transform_brc_params((uint8_t*)msg.data, msg.length, port->dx, port->dy, port->partner.rot);

  uint32_t fingerprint = grid_fingerprint_calculate(msg.data);

  if (msg.data[1] == GRID_CONST_BRC) {

    if (grid_fingerprint_buf_find(fpb, fingerprint)) {
      return;
    }

    grid_fingerprint_buf_store(fpb, fingerprint);
  }

  grid_port_recv_msg(port, (uint8_t*)msg.data, msg.length);
}

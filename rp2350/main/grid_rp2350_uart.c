#include "grid_rp2350_uart.h"

#include <assert.h>
#include <string.h>

#if GRID_RP2350_UART_DEBUG
#include <stdio.h>

#include "pico/time.h"
#endif

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/sync.h"

#include "uart_duplex.pio.h"

#include "grid_transport.h"

// Real UARTs already claimed (UART0=stdio, UART1=WS2812 LED) -- daisy-chain
// runs on PIO instead: one state machine per direction doing RX and TX together
// (see uart_duplex.pio).
#define GRID_RP2350_UART_PIO pio0

// RX frame ring per direction, in 32-bit frame words (one DMA write ring, size
// must match the aligned() below). The CPU converts frames to bytes in
// grid_rp2350_uart_rx_poll(), so it has to run at least once per
// RING_WORDS * 5.5 us (~11 ms) or the oldest frames are overwritten.
#define GRID_RP2350_UART_RX_RING_BITS 13
#define GRID_RP2350_UART_RX_RING_WORDS (1u << (GRID_RP2350_UART_RX_RING_BITS - 2))

// Endless mode never decrements the count, but a trigger with count 0 is a
// zero-length transfer and halts immediately -- keep the count non-zero.
#define GRID_RP2350_UART_DMA_ENDLESS ((DMA_CH0_TRANS_COUNT_MODE_VALUE_ENDLESS << DMA_CH0_TRANS_COUNT_MODE_LSB) | 1u)

// GPIO9 (WEST RX) doubles as stdio console RX (harmless, nothing reads stdin).
static const uint8_t grid_rp2350_uart_tx_pin[GRID_RP2350_UART_DIR_COUNT] = {22, 20, 10, 8};
static const uint8_t grid_rp2350_uart_rx_pin[GRID_RP2350_UART_DIR_COUNT] = {7, 21, 19, 9};

struct grid_uwsr_t grid_rp2350_uart_uwsr[GRID_RP2350_UART_DIR_COUNT];
uint8_t grid_rp2350_uart_tx_buf[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

// tx_buf bytes expanded to 11-bit PIO frames, fed by the TX DMA channel.
static uint32_t grid_rp2350_uart_tx_frames[GRID_RP2350_UART_DIR_COUNT][GRID_PARAMETER_SPI_TRANSACTION_length];

static uint32_t grid_rp2350_uart_rx_ring[GRID_RP2350_UART_DIR_COUNT][GRID_RP2350_UART_RX_RING_WORDS] __attribute__((aligned(1u << GRID_RP2350_UART_RX_RING_BITS)));

// Constant source for the idle DMA channel (11 ones = line held high). Kept in
// RAM, not flash, so flash operations can't stall the TX FIFO feed.
static uint32_t grid_rp2350_uart_idle_frame = UART_DUPLEX_TX_IDLE;

static uint grid_rp2350_uart_sm[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_tx_dma_chan[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_idle_dma_chan[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_rx_dma_chan[GRID_RP2350_UART_DIR_COUNT];

// Frame ring read index, and uwsr write index (bytes land in uwsr->data exactly
// where the old byte-DMA put them; zero = unwritten is what the reader relies on).
static uint32_t grid_rp2350_uart_rx_read[GRID_RP2350_UART_DIR_COUNT];
static int grid_rp2350_uart_uwsr_write[GRID_RP2350_UART_DIR_COUNT];
static bool grid_rp2350_uart_rx_stopped[GRID_RP2350_UART_DIR_COUNT];

#if GRID_RP2350_UART_DEBUG
// Debug: RX frames dropped for bad framing, good frames, and the deepest frame
// backlog seen by rx_poll (ring is RING_WORDS deep).
volatile uint32_t grid_rp2350_uart_framing_err[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_rx_ok[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_rx_backlog_max[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_tx_count[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_err_start[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_err_stop[GRID_RP2350_UART_DIR_COUNT];
volatile uint32_t grid_rp2350_uart_bad_frame[GRID_RP2350_UART_DIR_COUNT][3];
#endif

static inline uint32_t grid_rp2350_uart_rx_write_index(uint8_t dir) {
  uint32_t addr = dma_hw->ch[grid_rp2350_uart_rx_dma_chan[dir]].write_addr;
  return (addr - (uint32_t)grid_rp2350_uart_rx_ring[dir]) / sizeof(uint32_t);
}

// Errata RP2350-E5: the enable bit of the aborted channel and any channel
// chained to it must be cleared before aborting so it can't re-trigger.
static void __not_in_flash_func(grid_rp2350_uart_dma_abort)(int chan) {

  uint32_t values = chan << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB | 0u << DMA_CH0_CTRL_TRIG_EN_LSB;
  uint32_t write_mask = DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS | DMA_CH0_CTRL_TRIG_EN_BITS;
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, values, write_mask);

  dma_channel_abort(chan);
}

void __not_in_flash_func(grid_rp2350_uart_port_stop_dma)(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  grid_rp2350_uart_dma_abort(grid_rp2350_uart_rx_dma_chan[dir]);
  grid_rp2350_uart_rx_stopped[dir] = true;
}

void __not_in_flash_func(grid_rp2350_uart_port_reset_dma)(uint8_t dir) {

  grid_rp2350_uart_port_stop_dma(dir);

  int chan = grid_rp2350_uart_rx_dma_chan[dir];

  // Re-enable DMA channel after stopping so it responds to the initial trigger
  hw_write_masked(&dma_hw->ch[chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS, DMA_CH0_CTRL_TRIG_EN_BITS);

  // Start over: frames and the uwsr both begin at index 0, matching
  // grid_uwsr_init()'s read index of capacity - 1.
  grid_rp2350_uart_rx_read[dir] = 0;
  grid_rp2350_uart_uwsr_write[dir] = 0;

  dma_channel_set_write_addr(chan, grid_rp2350_uart_rx_ring[dir], false);
  dma_channel_set_trans_count(chan, GRID_RP2350_UART_DMA_ENDLESS, true);

  grid_rp2350_uart_rx_stopped[dir] = false;
}

void grid_rp2350_uart_rx_poll(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  if (grid_rp2350_uart_rx_stopped[dir]) {
    return;
  }

  struct grid_uwsr_t* uwsr = &grid_rp2350_uart_uwsr[dir];

  uint32_t write = grid_rp2350_uart_rx_write_index(dir);
  uint32_t read = grid_rp2350_uart_rx_read[dir];

#if GRID_RP2350_UART_DEBUG
  uint32_t backlog = (write - read) % GRID_RP2350_UART_RX_RING_WORDS;
  if (backlog > grid_rp2350_uart_rx_backlog_max[dir]) {
    grid_rp2350_uart_rx_backlog_max[dir] = backlog;
  }
#endif

  while (read != write) {

    uint32_t frame = grid_rp2350_uart_rx_ring[dir][read];
    read = (read + 1) % GRID_RP2350_UART_RX_RING_WORDS;

    if (!uart_duplex_frame_ok(frame)) {
#if GRID_RP2350_UART_DEBUG
      uint32_t n = grid_rp2350_uart_framing_err[dir]++;
      grid_rp2350_uart_err_start[dir] += !((frame >> 21) & 1u);
      grid_rp2350_uart_err_stop[dir] += (frame >> 31) & 1u;
      grid_rp2350_uart_bad_frame[dir][n % 3] = frame;
#endif
      continue;
    }

#if GRID_RP2350_UART_DEBUG
    ++grid_rp2350_uart_rx_ok[dir];
#endif
    uwsr->data[grid_rp2350_uart_uwsr_write[dir]] = uart_duplex_frame_data(frame);
    grid_rp2350_uart_uwsr_write[dir] = (grid_rp2350_uart_uwsr_write[dir] + 1) % uwsr->capacity;
  }

  grid_rp2350_uart_rx_read[dir] = read;
}

void __not_in_flash_func(grid_rp2350_uart_tx_start)(uint8_t dir, uint32_t size) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);
  assert(size > 0 && size <= GRID_PARAMETER_SPI_TRANSACTION_length);

  for (uint32_t i = 0; i < size; ++i) {
    grid_rp2350_uart_tx_frames[dir][i] = uart_duplex_tx_frame(grid_rp2350_uart_tx_buf[dir][i]);
  }

#if GRID_RP2350_UART_DEBUG
  ++grid_rp2350_uart_tx_count[dir];
#endif

  int chan = grid_rp2350_uart_tx_dma_chan[dir];
  int idle = grid_rp2350_uart_idle_dma_chan[dir];

  // The PIO stalls (and RX with it) if its TX FIFO runs dry, so swap idle ->
  // data with interrupts off; the 4-word FIFO holds ~22 us of idle meanwhile.
  uint32_t irq_state = save_and_disable_interrupts();

  grid_rp2350_uart_dma_abort(idle);
  hw_write_masked(&dma_hw->ch[idle].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS, DMA_CH0_CTRL_TRIG_EN_BITS);

  dma_channel_set_read_addr(chan, grid_rp2350_uart_tx_frames[dir], false);
  dma_channel_set_trans_count(chan, size, true);

  // Re-arm idle (without trigger): the data channel's completion chains to it.
  dma_channel_set_trans_count(idle, GRID_RP2350_UART_DMA_ENDLESS, false);

  restore_interrupts(irq_state);
}

bool grid_rp2350_uart_tx_busy(uint8_t dir) {

  assert(dir < GRID_RP2350_UART_DIR_COUNT);

  return dma_channel_is_busy(grid_rp2350_uart_tx_dma_chan[dir]);
}

void grid_rp2350_uart_init(void) {

  uint offset = pio_add_program(GRID_RP2350_UART_PIO, &uart_duplex_program);

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {

    assert(grid_uwsr_malloc(&grid_rp2350_uart_uwsr[dir], GRID_PORT_SWSR_SIZE, '\n') == 0);

    uint sm = pio_claim_unused_sm(GRID_RP2350_UART_PIO, true);
    grid_rp2350_uart_sm[dir] = sm;
    uart_duplex_program_init(GRID_RP2350_UART_PIO, sm, offset, grid_rp2350_uart_rx_pin[dir], grid_rp2350_uart_tx_pin[dir], GRID_PARAMETER_UART_baudrate);

    int tx_chan = dma_claim_unused_channel(true);
    int idle_chan = dma_claim_unused_channel(true);
    int rx_chan = dma_claim_unused_channel(true);
    grid_rp2350_uart_tx_dma_chan[dir] = tx_chan;
    grid_rp2350_uart_idle_dma_chan[dir] = idle_chan;
    grid_rp2350_uart_rx_dma_chan[dir] = rx_chan;

    uint tx_dreq = pio_get_dreq(GRID_RP2350_UART_PIO, sm, true);

    // Idle: endless constant idle frames. Keeps the TX FIFO fed whenever no
    // message is in flight.
    dma_channel_config idle_cfg = dma_channel_get_default_config(idle_chan);
    channel_config_set_transfer_data_size(&idle_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&idle_cfg, false);
    channel_config_set_write_increment(&idle_cfg, false);
    channel_config_set_dreq(&idle_cfg, tx_dreq);
    dma_channel_configure(idle_chan, &idle_cfg, &GRID_RP2350_UART_PIO->txf[sm], &grid_rp2350_uart_idle_frame, GRID_RP2350_UART_DMA_ENDLESS, false);

    // Data: one message of 11-bit frames, then chain back to idle.
    dma_channel_config tx_cfg = dma_channel_get_default_config(tx_chan);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    channel_config_set_dreq(&tx_cfg, tx_dreq);
    channel_config_set_chain_to(&tx_cfg, idle_chan);
    dma_channel_configure(tx_chan, &tx_cfg, &GRID_RP2350_UART_PIO->txf[sm], grid_rp2350_uart_tx_frames[dir], 0, false);

    // RX: endless 32-bit frame words into a write-wrapped ring.
    dma_channel_config rx_cfg = dma_channel_get_default_config(rx_chan);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, true);
    channel_config_set_ring(&rx_cfg, true, GRID_RP2350_UART_RX_RING_BITS);
    channel_config_set_dreq(&rx_cfg, pio_get_dreq(GRID_RP2350_UART_PIO, sm, false));
    dma_channel_configure(rx_chan, &rx_cfg, grid_rp2350_uart_rx_ring[dir], &GRID_RP2350_UART_PIO->rxf[sm], GRID_RP2350_UART_DMA_ENDLESS, true);

    // Start idle only after the data channel exists: from here on the FIFO is
    // never allowed to run dry.
    dma_channel_start(idle_chan);
  }
}

#if GRID_RP2350_UART_DEBUG
static void grid_rp2350_uart_debug_print(uint8_t dir) {

  uint sm = grid_rp2350_uart_sm[dir];
  int idle = grid_rp2350_uart_idle_dma_chan[dir];

  printf("uart%u bad_start=%lu bad_stop=%lu last_bad=%08lx %08lx %08lx\n", dir, (unsigned long)grid_rp2350_uart_err_start[dir], (unsigned long)grid_rp2350_uart_err_stop[dir],
         (unsigned long)grid_rp2350_uart_bad_frame[dir][0], (unsigned long)grid_rp2350_uart_bad_frame[dir][1], (unsigned long)grid_rp2350_uart_bad_frame[dir][2]);
  printf("uart%u ok=%lu err=%lu backlog_max=%lu tx=%lu | pc=%u txlvl=%u rxlvl=%u rxdma_w=%lu idle_busy=%d idle_cnt=%08lx\n", dir, (unsigned long)grid_rp2350_uart_rx_ok[dir],
         (unsigned long)grid_rp2350_uart_framing_err[dir], (unsigned long)grid_rp2350_uart_rx_backlog_max[dir], (unsigned long)grid_rp2350_uart_tx_count[dir],
         (unsigned)pio_sm_get_pc(GRID_RP2350_UART_PIO, sm), (unsigned)pio_sm_get_tx_fifo_level(GRID_RP2350_UART_PIO, sm), (unsigned)pio_sm_get_rx_fifo_level(GRID_RP2350_UART_PIO, sm),
         (unsigned long)grid_rp2350_uart_rx_write_index(dir), (int)dma_channel_is_busy(idle), (unsigned long)dma_hw->ch[idle].transfer_count);
}

void grid_rp2350_uart_debug_poll(void) {

  static uint32_t last_ms;
  uint32_t now_ms = to_ms_since_boot(get_absolute_time());
  if (now_ms - last_ms < 1000) {
    return;
  }
  last_ms = now_ms;

  for (uint8_t dir = 0; dir < GRID_RP2350_UART_DIR_COUNT; ++dir) {
    grid_rp2350_uart_debug_print(dir);
  }
}
#endif

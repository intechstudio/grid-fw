#include "grid_rp2350_encoder.h"

#include <assert.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"

#include "grid_platform.h"

#define GRID_RP2350_ENCODER_CS_PIN 1
#define GRID_RP2350_ENCODER_SCK_PIN 2
#define GRID_RP2350_ENCODER_MISO_PIN 4

struct grid_rp2350_encoder_model grid_rp2350_encoder_state;

static int grid_rp2350_encoder_dma_tx_chan;
static int grid_rp2350_encoder_dma_rx_chan;
static const uint8_t grid_rp2350_encoder_dummy_tx = 0x00;

// Re-arms both DMA channels for the next transfer_length-byte burst. The TX
// channel's read address never changes (same dummy byte re-read each time,
// just there to pace SCK), but the RX channel's write address must be reset
// back to the start of rx_buffer since it auto-increments during the burst.
static void grid_rp2350_encoder_arm_dma(struct grid_rp2350_encoder_model* enc) {

  dma_channel_set_trans_count(grid_rp2350_encoder_dma_tx_chan, enc->transfer_length, false);

  dma_channel_set_write_addr(grid_rp2350_encoder_dma_rx_chan, enc->rx_buffer, false);
  dma_channel_set_trans_count(grid_rp2350_encoder_dma_rx_chan, enc->transfer_length, false);

  dma_start_channel_mask((1u << grid_rp2350_encoder_dma_tx_chan) | (1u << grid_rp2350_encoder_dma_rx_chan));
}

// Fires once the RX DMA channel has captured transfer_length bytes. Pulses CS
// low (the shift registers' load pulse) then back high before restarting, per
// grid_d51_encoder.c's spi_transfer_complete_cb.
static void grid_rp2350_encoder_dma_irq(void) {

  struct grid_rp2350_encoder_model* enc = &grid_rp2350_encoder_state;

  dma_hw->ints1 = 1u << grid_rp2350_encoder_dma_rx_chan;

  gpio_put(GRID_RP2350_ENCODER_CS_PIN, 0);

  struct grid_encoder_result result = {
      .data = &enc->rx_buffer[1],
      .length = enc->transfer_length - 1,
  };
  enc->process_encoder(&result);

  gpio_put(GRID_RP2350_ENCODER_CS_PIN, 1);
  grid_rp2350_encoder_arm_dma(enc);
}

void grid_rp2350_encoder_init(struct grid_rp2350_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder) {

  enc->rx_buffer = grid_platform_allocate_volatile(transfer_length);
  memset(enc->rx_buffer, 0, transfer_length);

  enc->transfer_length = transfer_length;

  assert(process_encoder);
  enc->process_encoder = process_encoder;

  gpio_init(GRID_RP2350_ENCODER_CS_PIN);
  gpio_set_dir(GRID_RP2350_ENCODER_CS_PIN, GPIO_OUT);
  gpio_put(GRID_RP2350_ENCODER_CS_PIN, 1);

  spi_init(spi0, clock_rate);
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_set_function(GRID_RP2350_ENCODER_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(GRID_RP2350_ENCODER_MISO_PIN, GPIO_FUNC_SPI);

  grid_rp2350_encoder_dma_tx_chan = dma_claim_unused_channel(true);
  dma_channel_config tx_cfg = dma_channel_get_default_config(grid_rp2350_encoder_dma_tx_chan);
  channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&tx_cfg, false);
  channel_config_set_write_increment(&tx_cfg, false);
  channel_config_set_dreq(&tx_cfg, spi_get_dreq(spi0, true));
  dma_channel_configure(grid_rp2350_encoder_dma_tx_chan, &tx_cfg, &spi_get_hw(spi0)->dr, &grid_rp2350_encoder_dummy_tx, transfer_length, false);

  grid_rp2350_encoder_dma_rx_chan = dma_claim_unused_channel(true);
  dma_channel_config rx_cfg = dma_channel_get_default_config(grid_rp2350_encoder_dma_rx_chan);
  channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&rx_cfg, false);
  channel_config_set_write_increment(&rx_cfg, true);
  channel_config_set_dreq(&rx_cfg, spi_get_dreq(spi0, false));
  dma_channel_configure(grid_rp2350_encoder_dma_rx_chan, &rx_cfg, enc->rx_buffer, &spi_get_hw(spi0)->dr, transfer_length, false);

  // DMA_IRQ_0 is already claimed exclusively by grid_rp2350_adc.c -- this
  // uses the independent DMA_IRQ_1 line so both can coexist.
  dma_channel_set_irq1_enabled(grid_rp2350_encoder_dma_rx_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_1, grid_rp2350_encoder_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);
}

void grid_rp2350_encoder_start(struct grid_rp2350_encoder_model* enc) {

  gpio_put(GRID_RP2350_ENCODER_CS_PIN, 1);
  grid_rp2350_encoder_arm_dma(enc);
}

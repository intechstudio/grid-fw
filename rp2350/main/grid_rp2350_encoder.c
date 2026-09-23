#include "grid_rp2350_encoder.h"

#include <assert.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"

#include "grid_platform.h"

#define RP2350_PIN_ENCODER_CS 1
#define RP2350_PIN_ENCODER_SCK 2
#define RP2350_PIN_ENCODER_MISO 4

struct grid_rp2350_encoder_model grid_rp2350_encoder_state;

static int grid_rp2350_encoder_dma_tx_chan;
static int grid_rp2350_encoder_dma_rx_chan;

static void grid_rp2350_encoder_arm_dma(struct grid_rp2350_encoder_model* enc) {

  // Only the transaction length needs to be set for TX, its read address does
  // not need setting because TX is configured with read increment disabled.
  dma_channel_set_trans_count(grid_rp2350_encoder_dma_tx_chan, enc->rx_length, false);

  dma_channel_set_write_addr(grid_rp2350_encoder_dma_rx_chan, enc->rx_buffer, false);
  dma_channel_set_trans_count(grid_rp2350_encoder_dma_rx_chan, enc->rx_length, false);

  dma_start_channel_mask((1u << grid_rp2350_encoder_dma_tx_chan) | (1u << grid_rp2350_encoder_dma_rx_chan));
}

static void grid_rp2350_encoder_dma_irq(void) {

  struct grid_rp2350_encoder_model* enc = &grid_rp2350_encoder_state;

  // Clear channel interrupt by writing a bitmask
  dma_hw->ints1 = 1u << grid_rp2350_encoder_dma_rx_chan;

  // Drive SH/LDZ low to let the register load
  gpio_put(RP2350_PIN_ENCODER_CS, 0);

  // Offset by 1 to skip the HWCFG register
  struct grid_encoder_result result = {
      .data = &enc->rx_buffer[1],
      .length = enc->rx_length - 1,
  };
  enc->process_encoder(&result);

  // Drive SH/LDZ high to let the register shift
  gpio_put(RP2350_PIN_ENCODER_CS, 1);

  grid_rp2350_encoder_arm_dma(enc);
}

static const uint8_t grid_rp2350_encoder_dummy_tx = 0;

void grid_rp2350_encoder_init(struct grid_rp2350_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder) {

  enc->rx_length = transfer_length;
  enc->rx_buffer = grid_platform_allocate_volatile(enc->rx_length);
  memset(enc->rx_buffer, 0, enc->rx_length);

  assert(process_encoder);
  enc->process_encoder = process_encoder;

  gpio_init(RP2350_PIN_ENCODER_CS);
  gpio_set_dir(RP2350_PIN_ENCODER_CS, GPIO_OUT);
  gpio_put(RP2350_PIN_ENCODER_CS, 0);

  spi_init(spi0, clock_rate);
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_set_function(RP2350_PIN_ENCODER_SCK, GPIO_FUNC_SPI);
  gpio_set_function(RP2350_PIN_ENCODER_MISO, GPIO_FUNC_SPI);

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

  dma_channel_set_irq1_enabled(grid_rp2350_encoder_dma_rx_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_1, grid_rp2350_encoder_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);
}

void grid_rp2350_encoder_start(struct grid_rp2350_encoder_model* enc) {

  gpio_put(RP2350_PIN_ENCODER_CS, 1);
  grid_rp2350_encoder_arm_dma(enc);
}

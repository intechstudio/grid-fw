#include "grid_pico_spi.h"
#include "grid_pico_pins.h"

#include "../../grid_common/grid_protocol.h"

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

uint grid_pico_spi_dma_tx_channel;
uint grid_pico_spi_dma_rx_channel;
volatile uint8_t spi_rx_data_available = false;

int grid_pico_spi_is_rx_data_available(void) { return spi_rx_data_available; }

void grid_pico_spi_set_rx_data_available_flag(void) { spi_rx_data_available = 1; }

void grid_pico_spi_clear_rx_data_available_flag(void) { spi_rx_data_available = 0; }

int grid_pico_spi_isready(void) { return !dma_channel_is_busy(grid_pico_spi_dma_rx_channel); }

void spi_start_transfer(uint tx_channel, uint rx_channel, uint8_t* tx_buffer, uint8_t* rx_buffer, irq_handler_t callback) {

  spi_rx_data_available = false;
  gpio_put(GRID_PICO_PIN_SPI_CS, 0);

  dma_channel_config c = dma_channel_get_default_config(tx_channel);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
  dma_channel_configure(tx_channel, &c,
                        &spi_get_hw(spi_default)->dr,          // write address
                        tx_buffer,                             // read address
                        GRID_PARAMETER_SPI_TRANSACTION_length, // elements (of transfer_data_size each)
                        false);                                // don't start yet

  c = dma_channel_get_default_config(rx_channel);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  dma_channel_configure(rx_channel, &c,
                        rx_buffer,                             // write address
                        &spi_get_hw(spi_default)->dr,          // read address
                        GRID_PARAMETER_SPI_TRANSACTION_length, // elements (of transfer_data_size each)
                        false);                                // don't start yet

  if (callback != NULL) {
    dma_channel_set_irq0_enabled(rx_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, callback);
    irq_set_enabled(DMA_IRQ_0, true);
  }

  dma_start_channel_mask((1u << tx_channel) | (1u << rx_channel));
}

void grid_pico_spi_init(void) {
  // SPI INIT

  // Setup COMMON stuff
  gpio_init(GRID_PICO_PIN_SPI_CS);
  gpio_set_dir(GRID_PICO_PIN_SPI_CS, GPIO_OUT);
  gpio_put(GRID_PICO_PIN_SPI_CS, 1);

  uint baudrate = spi_init(spi_default, 31250 * 1000);

  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

  // Force loopback for testing (I don't have an SPI device handy)
  // hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

  grid_pico_spi_dma_tx_channel = dma_claim_unused_channel(true);
  grid_pico_spi_dma_rx_channel = dma_claim_unused_channel(true);
}

void grid_pico_spi_transfer(uint8_t* tx_buffer, uint8_t* rx_buffer) {
  spi_start_transfer(grid_pico_spi_dma_tx_channel, grid_pico_spi_dma_rx_channel, tx_buffer, rx_buffer, grid_pico_spi_dma_xfer_complete_cb);
}

void grid_pico_spi_dma_xfer_complete_cb(void) {

  spi_rx_data_available = true;
  gpio_put(GRID_PICO_PIN_SPI_CS, 1);
  dma_hw->ints0 = 1u << grid_pico_spi_dma_rx_channel;
}

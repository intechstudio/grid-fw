#include "grid_rp2350_adc.h"

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#define RP2350_PIN_ADC_FIRST 26
#define RP2350_ADC_CHAN_COUNT 4
#define RP2350_ADC_ROUND_ROBIN_MASK 0x0F

#define RP2350_PIN_MUX_A0 25
#define RP2350_PIN_MUX_A1 24

struct grid_rp2350_adc_model grid_rp2350_adc_state;

#define RP2350_ADC_OVERSAMPLE 8

// Number of encoded transfers, round-robin over 4 channels with oversampling
#define ADC_DWELL_SAMPLES (RP2350_ADC_CHAN_COUNT * RP2350_ADC_OVERSAMPLE)

static int grid_adc_dma_chan;

static inline void grid_rp2350_adc_mux_increment(struct grid_rp2350_adc_model* adc) { GRID_MUX_INCREMENT(adc->mux_index, adc->mux_positions_bm); }

static inline void grid_rp2350_adc_mux_write(struct grid_rp2350_adc_model* adc) {

  gpio_put(RP2350_PIN_MUX_A0, adc->mux_index & 1);
  gpio_put(RP2350_PIN_MUX_A1, (adc->mux_index >> 1) & 1);
}

static uint16_t grid_adc_dma_buffer[ADC_DWELL_SAMPLES];

static void grid_rp2350_adc_arm_dma(void) {

  dma_channel_set_write_addr(grid_adc_dma_chan, grid_adc_dma_buffer, false);
  dma_channel_set_trans_count(grid_adc_dma_chan, ADC_DWELL_SAMPLES, true);
}

static void grid_rp2350_adc_dma_irq(void) {

  struct grid_rp2350_adc_model* adc = &grid_rp2350_adc_state;

  // Clear channel interrupt by writing a bitmask
  dma_hw->ints0 = 1u << grid_adc_dma_chan;

  // Stop the free-running ADC
  adc_run(false);

  uint8_t mux_state = adc->mux_index;
  grid_rp2350_adc_mux_increment(adc);
  grid_rp2350_adc_mux_write(adc);

  for (int chan = 0; chan < RP2350_ADC_CHAN_COUNT; ++chan) {

    uint32_t sum = 0;
    for (int pass = 0; pass < RP2350_ADC_OVERSAMPLE; ++pass) {
      sum += grid_adc_dma_buffer[chan + pass * RP2350_ADC_CHAN_COUNT];
    }

    struct grid_adc_result result = {
        .channel = chan,
        .mux_state = mux_state,
        .value = sum / RP2350_ADC_OVERSAMPLE,
    };

    adc->process_analog(&result);
  }

  // Drain stray samples from the FIFO
  adc_fifo_drain();

  grid_rp2350_adc_start(adc);
}

void grid_rp2350_adc_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm, grid_process_analog_t process_analog) {

  assert(process_analog);
  adc->process_analog = process_analog;

  adc_init();

  for (uint8_t i = 0; i < RP2350_ADC_CHAN_COUNT; ++i) {
    adc_gpio_init(RP2350_PIN_ADC_FIRST + i);
  }

  // Enable multiple channels to be sampled in a round-robin fashion.
  adc_set_round_robin(RP2350_ADC_ROUND_ROBIN_MASK);

  // Enable writes to the FIFO and enable DMA reqs on it with a threshold of 1.
  adc_fifo_setup(true, true, 1, false, false);

  // DMA channel drains four samples (one per channel) then raises DMA_IRQ_0.
  grid_adc_dma_chan = dma_claim_unused_channel(true);
  dma_channel_config cfg = dma_channel_get_default_config(grid_adc_dma_chan);
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);
  dma_channel_configure(grid_adc_dma_chan, &cfg, grid_adc_dma_buffer, &adc_hw->fifo, ADC_DWELL_SAMPLES, false);

  dma_channel_set_irq0_enabled(grid_adc_dma_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_0, grid_rp2350_adc_dma_irq);
  irq_set_enabled(DMA_IRQ_0, true);
}

void grid_rp2350_adc_mux_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm) {

  gpio_init(RP2350_PIN_MUX_A0);
  gpio_set_dir(RP2350_PIN_MUX_A0, GPIO_OUT);

  gpio_init(RP2350_PIN_MUX_A1);
  gpio_set_dir(RP2350_PIN_MUX_A1, GPIO_OUT);

  adc->mux_positions_bm = mux_positions_bm;

  GRID_MUX_FIRST_VALID(adc->mux_index, adc->mux_positions_bm);

  grid_rp2350_adc_mux_write(adc);
}

void grid_rp2350_adc_start(struct grid_rp2350_adc_model* adc) {

  adc_select_input(0);
  grid_rp2350_adc_arm_dma();
  adc_run(true);
}

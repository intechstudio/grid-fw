#include "grid_rp2350_adc.h"

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "grid_ain.h"

#define GRID_ADC_FIRST_PIN 26   // AIN0..3 live on GPIO26..29 (RP2350A / Pico 2)
#define GRID_ADC_NUM_CHANNELS 4 // four ADC common lines, one per 74HC4052 pair
#define GRID_ADC_ROUND_ROBIN_MASK 0x0F

// Shared 74HC4052 address lines: A1:A0 select 1 of 4 mux positions.
#define GRID_MUX_A0_PIN 25
#define GRID_MUX_A1_PIN 24

struct grid_rp2350_adc_model grid_rp2350_adc_state;

// One full dwell at a mux position captures OVERSAMPLE round-robin passes over
// the four channels, laid out as [pass0: ch0..3][pass1: ch0..3]...
#define GRID_ADC_DWELL_SAMPLES (GRID_ADC_NUM_CHANNELS * GRID_RP2350_ADC_OVERSAMPLE)

static int grid_adc_dma_chan;
static uint16_t grid_adc_dma_buffer[GRID_ADC_DWELL_SAMPLES];

static void grid_rp2350_adc_mux_write(uint8_t index) {
  gpio_put(GRID_MUX_A0_PIN, index & 1);
  gpio_put(GRID_MUX_A1_PIN, (index >> 1) & 1);
}

// Point the DMA at the sample buffer and start it counting down four transfers.
// The DMA is DREQ-paced by the ADC FIFO, so it does not move until conversions
// actually complete.
static void grid_rp2350_adc_arm_dma(void) {
  dma_channel_set_write_addr(grid_adc_dma_chan, grid_adc_dma_buffer, false);
  dma_channel_set_trans_count(grid_adc_dma_chan, GRID_ADC_DWELL_SAMPLES, true);
}

// Fires once per sweep, after the DMA has captured exactly four samples.
static void grid_rp2350_adc_dma_irq(void) {

  struct grid_rp2350_adc_model* adc = &grid_rp2350_adc_state;

  // Acknowledge the DMA completion interrupt.
  dma_hw->ints0 = 1u << grid_adc_dma_chan;

  // Stop the free-running ADC so it sits idle while we switch the mux, then
  // flush any conversion that overshot into the FIFO after the DMA finished.
  adc_run(false);
  adc_fifo_drain();

  uint8_t mux_state = adc->mux_index;

  for (uint8_t channel = 0; channel < GRID_ADC_NUM_CHANNELS; channel++) {

    // Average the OVERSAMPLE passes for this channel (indices channel, channel +
    // NUM_CHANNELS, channel + 2*NUM_CHANNELS, ...). Sum <= 8*4095 fits in 16 bits;
    // the mean stays a right-aligned 12-bit value (0..4095), sqrt(8) less noise.
    uint32_t acc = 0;
    for (uint32_t pass = 0; pass < GRID_RP2350_ADC_OVERSAMPLE; pass++) {
      acc += grid_adc_dma_buffer[channel + pass * GRID_ADC_NUM_CHANNELS];
    }

    struct grid_adc_result result = {
        .channel = channel,
        .mux_state = mux_state,
        .value = acc / GRID_RP2350_ADC_OVERSAMPLE,
    };

    adc->process_analog(&result);
  }

  // Advance to the next mux position and let it settle during the remainder of
  // this ISR while the ADC is idle -- no throwaway conversion required.
  GRID_MUX_INCREMENT(adc->mux_index, adc->mux_positions_bm);
  grid_rp2350_adc_mux_write(adc->mux_index);

  // Realign the round-robin to channel 0 and explicitly start the next sweep.
  adc_select_input(0);
  grid_rp2350_adc_arm_dma();
  adc_run(true);
}

void grid_rp2350_adc_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm, grid_process_analog_t process_analog) {

  adc->process_analog = process_analog;

  adc_init();
  for (uint8_t ch = 0; ch < GRID_ADC_NUM_CHANNELS; ch++) {
    adc_gpio_init(GRID_ADC_FIRST_PIN + ch);
  }
  adc_set_round_robin(GRID_ADC_ROUND_ROBIN_MASK);

  // FIFO feeds the DMA one sample at a time via DREQ (threshold of 1).
  adc_fifo_setup(true, true, 1, false, false);

  // DMA channel drains four samples (one per channel) then raises DMA_IRQ_0.
  grid_adc_dma_chan = dma_claim_unused_channel(true);
  dma_channel_config cfg = dma_channel_get_default_config(grid_adc_dma_chan);
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);
  dma_channel_configure(grid_adc_dma_chan, &cfg, grid_adc_dma_buffer, &adc_hw->fifo, GRID_ADC_DWELL_SAMPLES, false);

  dma_channel_set_irq0_enabled(grid_adc_dma_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_0, grid_rp2350_adc_dma_irq);
  irq_set_enabled(DMA_IRQ_0, true);

  // Mux address lines as outputs (the mux hardware init).
  gpio_init(GRID_MUX_A0_PIN);
  gpio_set_dir(GRID_MUX_A0_PIN, GPIO_OUT);
  gpio_init(GRID_MUX_A1_PIN);
  gpio_set_dir(GRID_MUX_A1_PIN, GPIO_OUT);
}

void grid_rp2350_adc_mux_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm) {

  adc->mux_positions_bm = mux_positions_bm;
  GRID_MUX_FIRST_VALID(adc->mux_index, adc->mux_positions_bm);
  grid_rp2350_adc_mux_write(adc->mux_index);
}

void grid_rp2350_adc_start(struct grid_rp2350_adc_model* adc) {

  adc_select_input(0);
  grid_rp2350_adc_arm_dma();
  adc_run(true);
}

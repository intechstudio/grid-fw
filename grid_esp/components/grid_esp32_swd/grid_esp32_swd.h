

#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


#define SWD_CLOCK_PERIOD 0


void swd_dummy_clock(void);

void swd_write_raw(uint32_t data, uint8_t length);

void swd_write(uint32_t data, uint8_t length);

void swd_linereset();

void swd_switch_from_jtag_to_swd();

void swd_turnround_target_next();

void swd_turnround_host_next();

uint8_t swd_read_acknowledge();

void swd_target_select(uint8_t core_id);

uint32_t swd_read(uint8_t length);

uint32_t swd_read_idcode();

uint32_t swd_read_dlcr();

void swd_write_select(uint32_t value);

void swd_write_abort(uint32_t value);

void swd_write_ctrlstat(uint32_t value);

void swd_write_apc(uint32_t value);

void swd_write_ap0(uint32_t value);

void swd_write_ap4(uint32_t value);

void swd_write_ap8(uint32_t value);

uint32_t swd_read_ctrlstat();

uint32_t swd_read_buff();

uint32_t swd_read_apc();


uint32_t swd_read_ap0();

uint32_t swd_read_ap4();

uint32_t swd_read_ap8();

void swd_idle();



void grid_esp32_swd_pico_pins_init(uint8_t swclk_pin, uint8_t swdio_pin, uint8_t clock_pin);
void grid_esp32_swd_pico_clock_init(uint8_t timer_instance, uint8_t channel_instance);
void grid_esp32_swd_pico_program_sram(uint8_t swclk_pin, uint8_t swdio_pin, uint8_t* buffer, uint32_t length);


#ifdef __cplusplus
}
#endif

/*
 * grid_d51.h
 *
 * Created: 6/3/2020 5:02:04 PM
 *  Author: WPC-User
 */

#ifndef GRID_D51_H_
#define GRID_D51_H_

#include "grid_d51_module.h"
#include <hpl_user_area.h>

#include "sam.h"

#include "grid_d51_led.h"
#include "grid_d51_nvm.h"
#include "grid_d51_uart.h"

#define GRID_D51_USER_ROW_BASE 0x804000

#define GRID_D51_DEMCR (*(uint32_t*)0xE000EDFC)
#define GRID_D51_DWT_CTRL (*(uint32_t*)0xE0001000)
#define GRID_D51_DWT_CYCCNT (*(uint32_t*)0xE0001004)

/* ==================== Reading MCU Unique Serial Number ======================
 */
// Word 0: 0x008061FC	Word 1: 0x00806010	Word 2: 0x00806014	Word 3:
// 0x00806018

#define GRID_D51_UNIQUE_ID_ADDRESS_0 0x008061FC
#define GRID_D51_UNIQUE_ID_ADDRESS_1 0x00806010
#define GRID_D51_UNIQUE_ID_ADDRESS_2 0x00806014
#define GRID_D51_UNIQUE_ID_ADDRESS_3 0x00806018

void grid_d51_init();

void grid_d51_verify_user_row();

uint8_t grid_d51_boundary_scan(uint32_t* result_bitmap);
void grid_d51_boundary_scan_report(uint32_t* result_bitmap);

uint32_t grid_d51_dwt_enable();
uint32_t grid_d51_dwt_cycles_read();

uint8_t grid_fusb302_read_id(struct io_descriptor* i2c_io);
uint8_t grid_mxt144u_read_id(struct io_descriptor* i2c_io);

void grid_d51_nvic_debug_priorities(void);

void grid_d51_nvic_set_interrupt_priority(IRQn_Type irqn, uint32_t priority);
uint32_t grid_d51_nvic_get_interrupt_priority(IRQn_Type irqn);

// interrupts with priority number higher or equal to the given priority
// parameter will not be served. set to 0 to enable all interrupts
void grid_d51_nvic_set_interrupt_priority_mask(uint32_t priority);
uint32_t grid_d51_nvic_get_interrupt_priority_mask(void);

void grid_platform_printf(char const* fmt, ...);

uint32_t grid_platform_get_id(uint32_t* return_array);
uint32_t grid_platform_get_hwcfg();
uint8_t grid_platform_get_random_8();
void grid_platform_delay_ms(uint32_t delay_milliseconds);
uint8_t grid_platform_get_reset_cause();

uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length);
uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);

uint8_t grid_platform_get_nvm_state();

void grid_platform_clear_all_actionstring_files_from_page(uint8_t page);
void grid_platform_delete_actionstring_files_all();

int grid_platform_find_file(char* path, union grid_ui_file_handle* file_handle);
uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle);
int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size);
int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size);
int grid_platform_delete_file(union grid_ui_file_handle* file_handle);

void grid_platform_system_reset();
void grid_platform_nvm_defrag();

uint8_t grid_platform_get_adc_bit_depth();

void grid_platform_rtc_set_micros(uint64_t mic);
uint64_t grid_platform_rtc_get_micros(void);

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

uint32_t grid_platform_get_cycles();

uint32_t grid_platform_get_cycles_per_us();

void* grid_platform_allocate_volatile(size_t size);

#endif /* GRID_D51_H_ */

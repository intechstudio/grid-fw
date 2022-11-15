/*
 * grid_d51.h
 *
 * Created: 6/3/2020 5:02:04 PM
 *  Author: WPC-User
 */ 


#ifndef GRID_D51_H_
#define GRID_D51_H_

#include <hpl_user_area.h>
#include "grid_module.h"



#define GRID_D51_TASK_SUBTASK_count 10
#define GRID_D51_TASK_NAME_length 10


struct grid_d51_task{

	uint8_t taskname[GRID_D51_TASK_NAME_length];

    uint8_t startcount;

    uint8_t subtask;

    uint32_t t1;

	uint8_t subtaskcount;

	uint32_t min[GRID_D51_TASK_SUBTASK_count];
	uint32_t max[GRID_D51_TASK_SUBTASK_count];
	uint32_t sum[GRID_D51_TASK_SUBTASK_count];

};

void grid_d51_task_init(struct grid_d51_task* task, uint8_t* name);

void grid_d51_task_start(struct grid_d51_task* task);
void grid_d51_task_next(struct grid_d51_task* task);
void grid_d51_task_stop(struct grid_d51_task* task);

void grid_d51_task_clear(struct grid_d51_task* task);



#define  GRID_D51_USER_ROW_BASE 0x804000

#define  GRID_D51_DEMCR      (*(uint32_t *)0xE000EDFC)
#define  GRID_D51_DWT_CTRL   (*(uint32_t *)0xE0001000)
#define  GRID_D51_DWT_CYCCNT (*(uint32_t *)0xE0001004)


/* ==================== Reading MCU Unique Serial Nuber ====================== */
// Word 0: 0x008061FC	Word 1: 0x00806010	Word 2: 0x00806014	Word 3: 0x00806018

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

uint8_t grid_fusb302_read_id(struct io_descriptor * i2c_io);
uint8_t grid_mxt144u_read_id(struct io_descriptor * i2c_io);

void grid_d51_nvic_debug_priorities(void);

void grid_d51_nvic_set_interrupt_priority(IRQn_Type irqn, uint32_t priority);
uint32_t grid_d51_nvic_get_interrupt_priority(IRQn_Type irqn);

// interrupts with priority number higher or equal to the given priority parameter will not be served. set to 0 to enable all interrupts
void grid_d51_nvic_set_interrupt_priority_mask(uint32_t priority);
uint32_t grid_d51_nvic_get_interrupt_priority_mask(void);


uint32_t grid_d51_get_id(uint32_t* return_array);
uint32_t grid_d51_get_hwcfg();

uint8_t grid_d51_get_random_8();
uint8_t grid_d51_get_reset_cause();

#endif /* GRID_D51_H_ */
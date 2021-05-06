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




#define  GRID_D51_USER_ROW_BASE 0x804000

#define  GRID_D51_DEMCR      (*(uint32_t *)0xE000EDFC)
#define  GRID_D51_DWT_CTRL   (*(uint32_t *)0xE0001000)
#define  GRID_D51_DWT_CYCCNT (*(uint32_t *)0xE0001004)


void grid_d51_init();

void grid_d51_verify_user_row();


uint8_t grid_d51_boundary_scan(uint32_t* result_bitmap);
void grid_d51_boundary_scan_report(uint32_t* result_bitmap);

uint32_t grid_d51_dwt_enable();
uint32_t grid_d51_dwt_cycles_read();

uint8_t grid_fusb302_read_id(struct io_descriptor * i2c_io);
uint8_t grid_mxt144u_read_id(struct io_descriptor * i2c_io);


#endif /* GRID_D51_H_ */
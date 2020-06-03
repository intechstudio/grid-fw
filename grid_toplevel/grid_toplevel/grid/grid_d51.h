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

void grid_d51_bitmap_write_bit(uint8_t* buffer, uint8_t offset, uint8_t value, uint8_t* changed);

void grid_d51_init();

void grid_d51_verify_user_row();



#endif /* GRID_D51_H_ */
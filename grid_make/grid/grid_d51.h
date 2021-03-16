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

void grid_d51_init();

void grid_d51_verify_user_row();

uint8_t grid_d51_boundary_scan();




#endif /* GRID_D51_H_ */
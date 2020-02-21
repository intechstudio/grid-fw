/*
 * grid_hardwaretest.h
 *
 * Created: 2/20/2020 3:51:04 PM
 *  Author: WPC-User
 */ 


#ifndef GRID_HARDWARETEST_H_
#define GRID_HARDWARETEST_H_

#include "grid_led.h"

#include "grid_module.h"


void grid_hardwaretest_main();

void grid_hardwaretest_led_test_init(struct grid_led_model* mod, uint8_t num);


void grid_hardwaretest_led_test(struct grid_led_model* mod, uint32_t loop);


void grid_hardwaretest_port_test(uint32_t loop);


#endif /* GRID_HARDWARETEST_H_ */
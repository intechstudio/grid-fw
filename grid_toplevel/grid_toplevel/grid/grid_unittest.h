/*
 * grid_unittest.h
 *
 * Created: 2/19/2020 4:17:20 PM
 *  Author: WPC-User
 */ 


#ifndef GRID_UNITTEST_H_
#define GRID_UNITTEST_H_

#define GRID_UNITTEST_GROUP_NAME_LENGTH 32
#define GRID_UNITTEST_CASE_NAME_LENGTH 32

#define GRID_UNITTEST_COMMENT_LENGTH 16

#include "grid_module.h"

struct grid_unittest_model{


	char case_name[GRID_UNITTEST_GROUP_NAME_LENGTH];

	uint32_t case_pass;
	uint32_t case_fail;
	uint32_t case_done;
	uint32_t case_count;


	char group_name[GRID_UNITTEST_GROUP_NAME_LENGTH];
	
	uint32_t group_pass;
	uint32_t group_fail;
	uint32_t group_done;
	uint32_t group_count;
	
	
	uint32_t total_pass;
	uint32_t total_fail;
	
	
	
};

struct grid_unittest_model grid_unittest_state;

uint8_t grid_unittest_init(struct grid_unittest_model* mod);


uint8_t grid_unittest_group_init(struct grid_unittest_model* mod, char* gname);
uint8_t grid_unittest_group_done(struct grid_unittest_model* mod);


uint8_t grid_unittest_case_init(struct grid_unittest_model* mod, char* cname);


uint8_t grid_unittest_case_pass(struct grid_unittest_model* mod, char* comment);
uint8_t grid_unittest_case_fail(struct grid_unittest_model* mod, char* comment);


uint8_t grid_unittest_succes_counter;
uint8_t grid_unittest_failure_counter;


void grid_unittest_start(void);


#endif /* GRID_UNITTEST_H_ */
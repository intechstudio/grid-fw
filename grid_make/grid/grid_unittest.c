/*
 * grid_unittest.c
 *
 * Created: 2/19/2020 4:12:55 PM
 *  Author: WPC-User
 */ 


#include "grid_unittest.h"
	
uint8_t grid_unittest_init(struct grid_unittest_model* mod){
	
	for (uint32_t i=0; i<GRID_UNITTEST_CASE_NAME_LENGTH; i++)
	{
		mod->case_name[i] = 0;
	}
	
	mod->case_done = 1;
	mod->case_count = 0;
	mod->case_pass = 0;
	mod->case_fail = 0;
	
	
	
	
	for (uint32_t i=0; i<GRID_UNITTEST_GROUP_NAME_LENGTH; i++)
	{
		mod->group_name[i] = 0;
	}	
	
	mod->group_done = 1;
	mod->group_count = 0;
	mod->group_pass = 0;
	mod->group_fail = 0;
	
	
}	
	
	
uint8_t grid_unittest_group_init(struct grid_unittest_model* mod, char* gname){
	
	if (mod->group_done == 1){
		
		for (uint32_t i=0; i<GRID_UNITTEST_GROUP_NAME_LENGTH; i++)
		{		
			mod->group_name[i] = 0;		
		}
		
		for (uint32_t i=0; i<GRID_UNITTEST_GROUP_NAME_LENGTH-1; i++)
		{
			
			if(gname[i]==0){
				break;
			}
			else{
				mod->group_name[i] = gname[i];
			}
			
		}

		mod->group_name[GRID_UNITTEST_GROUP_NAME_LENGTH-1] = 0;
	
		printf(" ===== %s =====\r\n", mod->group_name);
		
		mod->case_pass = 0;
		mod->case_fail = 0;
		
		mod->group_done = 0;
		
	}
	else{
		while(1){
			//TRAP
		}
	}
	
	return 1;

}

uint8_t grid_unittest_group_done(struct grid_unittest_model* mod){
	
	if (mod->group_done == 0){
				
		if (mod->case_fail == 0){
			
			mod->group_pass++;
		}
		else{
			
			mod->group_fail++;
		}
		
		
		
			
		//printf("\r\n PASS: %d, FAIL: %d\r\n\r\n", mod->case_pass, mod->case_fail);
		
		
		printf("\r\n");
		
		
		
		mod->group_done = 1;
		
	}
	else{
		while(1){
			//TRAP
		}
	}
	
	
}


uint8_t grid_unittest_case_init(struct grid_unittest_model* mod, char* cname){
	
	if (mod->case_done == 1){
		
		for (uint32_t i=0; i<GRID_UNITTEST_CASE_NAME_LENGTH; i++)
		{		
			mod->case_name[i] = 0;		
		}
		
		for (uint32_t i=0; i<GRID_UNITTEST_CASE_NAME_LENGTH-1; i++)
		{
			
			if(cname[i]==0){
				break;
			}
			else{
				mod->case_name[i] = cname[i];
			}
			
		}

		mod->case_name[GRID_UNITTEST_CASE_NAME_LENGTH-1] = 0;
	
		
		
		mod->case_done = 0;
		
	}
	else{
		while(1){
			//TRAP
		}
	}
	
	
	return 1;

}


uint8_t grid_unittest_case_pass(struct grid_unittest_model* mod, char* comment){
	
	if (strlen(comment)){
		printf(" PASS: %s || %s\r\n", mod->case_name, comment);
		
	}
	else{
		
		printf(" PASS: %s \r\n", mod->case_name);
	}

	mod->case_pass++;
	mod->case_done = 1;
	
}
uint8_t grid_unittest_case_fail(struct grid_unittest_model* mod, char* comment){

	if (strlen(comment)){
		
		printf(" FAIL: %s || %s\r\n", mod->case_name, comment);
		
	}
	else{
		
		printf(" FAIL: %s \r\n", mod->case_name);
	}

	mod->case_fail++;
	mod->case_done = 1;
}






	
	
	
void grid_unittest_start(){
	
	
	grid_unittest_init(&grid_unittest_state);
	
	printf("\r\n Grid Unit Test Start\r\n\r\n");
	

	
	

	
	
}
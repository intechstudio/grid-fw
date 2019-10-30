#ifndef GRID_AIN_H_INCLUDED
#define GRID_AIN_H_INCLUDED

#include "grid/grid_module.h"

struct AIN_Channel
{

	uint16_t* buffer;
	uint8_t buffer_depth;
	
	
	
	uint8_t  result_format;
	uint8_t  result_resolution;
	uint16_t result_value;
	uint16_t result_average;
	uint16_t result_changed;
	
};


struct AIN_Channel* ain_channel_buffer;

#endif /* GRID_AIN_H_INCLUDED */
#ifndef GRID_AIN_H_INCLUDED
#define GRID_AIN_H_INCLUDED

#include "grid/grid_module.h"

#define GRID_AIN_MAXVALUE ((1<<16)-1)

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

uint8_t grid_ain_channel_init(struct AIN_Channel* instance , uint8_t buffer_depth);

uint8_t grid_ain_channel_deinit(struct AIN_Channel* instance);


/** Initialize ain buffer for a given number of analog channels */
uint8_t grid_ain_init(uint8_t length, uint8_t depth);

uint8_t grid_ain_add_sample(uint8_t channel, uint16_t value, uint8_t resolution);

uint8_t grid_ain_get_changed(uint8_t channel);
uint16_t grid_ain_get_average(uint8_t channel);


#endif /* GRID_AIN_H_INCLUDED */
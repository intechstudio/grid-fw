/*
 * grid_ain.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_ain.h"

struct grid_ain_model grid_ain_state;

uint32_t grid_ain_abs(int32_t value){

	if (value>0){
		return value;
	}
	else{
		return -value;	
	}
}

uint8_t grid_ain_channel_init(struct grid_ain_model* mod, uint8_t channel, uint8_t buffer_depth){
	
	mod->channel_buffer[channel].buffer_depth = buffer_depth;
	
	mod->channel_buffer[channel].result_average = 0;
	
	mod->channel_buffer[channel].buffer = malloc(mod->channel_buffer[channel].buffer_depth * sizeof(uint16_t));
	
	// Init the whole buffer with zeros
	for(uint8_t i=0; i<mod->channel_buffer[channel].buffer_depth; i++){
		mod->channel_buffer[channel].buffer[i] = 0;
	}
	
	mod->channel_buffer[channel].result_changed = 0;
	mod->channel_buffer[channel].result_value = 0;
		
	return 0;
}

uint8_t grid_ain_channel_deinit(struct grid_ain_model* mod, uint8_t channel){
	
	while(1) {
		//TRAP
	}
}


/** Initialize ain buffer for a given number of analog channels */
uint8_t grid_ain_init(struct grid_ain_model* mod, uint8_t length, uint8_t depth){
	
	// 2D buffer, example: 16 potentiometers, last 32 samples stored for each
	mod->channel_buffer = (struct AIN_Channel*) malloc(length * sizeof(struct AIN_Channel));
	mod->channel_buffer_length = length;

	for (uint8_t i=0; i<length; i++){
		grid_ain_channel_init(mod, i, depth);
	}

	return 0;
}

uint8_t grid_ain_add_sample(struct grid_ain_model* mod, uint8_t channel, uint16_t value, uint8_t source_resolution, uint8_t result_resolution){
	
	

	struct AIN_Channel* instance = &mod->channel_buffer[channel];
	
	uint32_t sum = 0;
	uint16_t minimum = -1; // -1 trick to get the largest possible number
	uint16_t maximum = 0;

	uint8_t minimum_index = 0;
	uint8_t maximum_index = 0;
	
	for (uint8_t i = 0; i<instance->buffer_depth; i++){
	
		uint16_t current = instance->buffer[i];
		
		sum += current;
		
		if (current > maximum){
			maximum = current;
			maximum_index = i;
		}
		
		if (current < minimum){
			minimum = current;
			minimum_index = i;
		}
	
	}
	
	uint16_t average = sum/instance->buffer_depth;
	
	if (value>average){		
		// Replace minimum in the buffer and recalculate sum
		sum = sum - instance->buffer[minimum_index] + value;
		instance->buffer[minimum_index] = value;		
	}else{
		// Replace maximum in the buffer and recalculate sum
		sum = sum - instance->buffer[maximum_index] + value;
		instance->buffer[maximum_index] = value;
	}
	
	// Recalculate average
	average = sum/instance->buffer_depth;
	

	// up until here all looks good, everything is 16 bit
	
	uint8_t downscale_factor = (source_resolution-result_resolution);
	uint8_t upscale_factor   = (source_resolution-result_resolution);
	
	
	uint16_t downsampled = (average>>downscale_factor);
	uint16_t upscaled    = downsampled<<upscale_factor;
	
	uint8_t criteria_a = instance->result_value != upscaled;

	uint8_t criteria_b = grid_ain_abs(instance->result_average - average)>(1<<downscale_factor);
	uint8_t criteria_c = upscaled > ((1<<source_resolution)-(1<<upscale_factor) -1);
	uint8_t criteria_d = upscaled==0;
	
	if (criteria_a && (criteria_b || criteria_c || criteria_d)){
		
		//printf("%d: %d %d\r\n", channel, average, upscaled);
		instance->result_average = average;
		instance->result_value = upscaled;
		instance->result_changed = 1;
		return 1;
	}else{		
		return 0;
	}
	
}

uint8_t grid_ain_get_changed(struct grid_ain_model* mod, uint8_t channel){
	
	struct AIN_Channel* instance = &mod->channel_buffer[channel];

	if (instance->result_changed){

		return 1;
	}
	else{

		return 0;
	}
}
	
uint16_t grid_ain_get_average(struct grid_ain_model* mod, uint8_t channel){
	
	struct AIN_Channel* instance = &mod->channel_buffer[channel];	
	
	instance->result_changed = 0;

	return instance->result_value;

}


int32_t grid_ain_get_average_scaled(struct grid_ain_model* mod, uint8_t channel, uint8_t source_resolution, uint8_t result_resolution, int32_t min, int32_t max){

	struct AIN_Channel* instance = &mod->channel_buffer[channel];	
	
	instance->result_changed = 0;


	uint16_t range_max = ((1<<source_resolution) - 1) - (1<<(source_resolution-result_resolution));

	int32_t next = instance->result_value * (max - min) / range_max + min;

	return next;

}

/*
 * grid_tel.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_tel.h"

uint32_t grid_tel_system_uptime    = 0;


struct TEL_event_counter{
	
	uint32_t absolute_counter;
	uint32_t frequency_counter;
	uint32_t frequency_reference;
	uint32_t frequency_alert;
	
	
	uint32_t frequency;
	
	struct TEL_event_counter* next;
		
};

struct TEL_event_counter* grid_tel_event_head = NULL;




// Register that the event happened and provide quick alert
uint8_t grid_tel_event_handler(struct TEL_event_counter* telemetry_event){
	
	if(telemetry_event == NULL) return 1;
	
	
	
	if(telemetry_event->absolute_counter<(uint32_t)(-1)){
		telemetry_event->absolute_counter++;
	}
	
	if(telemetry_event->frequency_counter<(uint32_t)(-1)){
		telemetry_event->frequency_counter++;
	}
	
	// Instant feedback
	if (telemetry_event->frequency_counter > telemetry_event->frequency_alert){
		return 1;
	}
	else{
		return 0;
	}
}  


struct TEL_event_counter* grid_tel_event_register(uint32_t frequency_reference, uint32_t frequency_alert){
	
	struct TEL_event_counter* new_item = (struct TEL_event_counter*) malloc(sizeof(struct TEL_event_counter));
	if(new_item != NULL){	// malloc successful
		
		new_item->next = NULL;
		
		new_item->absolute_counter = 0;
		new_item->frequency = 0;
		new_item->frequency_alert = frequency_alert;
		new_item->frequency_counter = 0;
		new_item->frequency_reference = frequency_reference;
		
		

		if(grid_tel_event_head == NULL){ //List is empty
			grid_tel_event_head = new_item;
		}else{
			
			struct TEL_event_counter* current = grid_tel_event_head;
			
			while (current->next != NULL)
			{
				current = current->next;
			}
			
			current -> next = new_item;
			
		}


	}

	return new_item;

}

uint8_t grid_tel_event_alert_status(struct TEL_event_counter* telemetry_event){
	
	return telemetry_event->frequency > telemetry_event->frequency_alert;


	
}


uint8_t grid_tel_calculate_event_frequency(struct TEL_event_counter* telemetry_event){

	// Save valid frequency data
	telemetry_event->frequency = telemetry_event->frequency_counter;
	
	
	//Update the freq counter variable
	if(telemetry_event->frequency_counter >= telemetry_event->frequency_reference){
		telemetry_event->frequency_counter -= telemetry_event->frequency_reference;
	}else{
		telemetry_event->frequency_counter = 0;
	}
	

	// Instant alert feedback
	if (telemetry_event->frequency_counter > telemetry_event->frequency_alert){
		return 1;
	}
	else{
		return 0;
	}	
}


void grid_tel_frequency_tick(){
	
	struct TEL_event_counter* current = grid_tel_event_head;
	while(current != NULL){
		grid_tel_calculate_event_frequency(current);
		current = current->next;
	}
	
};
            

#include "grid_module.h"



uint8_t grid_sync_mode_register[2] = {0, 0};

//====================== GRID ENCODER K+F ===================================//



	
	
void grid_sync_set_mode(enum grid_sync_selector sync_select, enum grid_sync_mode sync_mode){
	
	grid_sync_mode_register[sync_select - 1]  = sync_mode;
	
	if (sync_select == GRID_SYNC_1){		
		
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){	
			
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	
}

enum grid_sync_mode grid_sync_get_mode(enum grid_sync_selector sync_select){
	
	if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_MASTER){
		return GRID_SYNC_MASTER;
	}
	else if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_SLAVE){
		return GRID_SYNC_SLAVE;
	}
	else{
		return GRID_SYNC_INITIAL;	
	}	
}

void grid_sync_set_level(enum grid_sync_selector sync_select, uint8_t sync_level){
	
	if (sync_select == GRID_SYNC_1){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, sync_level);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, sync_level);
		}
		
	}
		
}



	





	
#define GRID_ADC_CFG_REVERSED	0
#define GRID_ADC_CFG_BINARY		1

	
uint8_t grid_adc_cfg[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	
	
void grid_adc_set_config(uint8_t register_offset, uint8_t bit_offest, uint8_t value){
		
	if (value){
		grid_adc_cfg[register_offset] |= (1<<bit_offest);
	}
	else{
		grid_adc_cfg[register_offset] &= ~(1<<bit_offest);
	}
		
}
	
void grid_adc_set_config_default(uint8_t register_offset){
		
	grid_adc_cfg[register_offset] = 0;
}	
	
uint8_t grid_adc_get_config(uint8_t register_offset, uint8_t bit_offest){
		
	return (grid_adc_cfg[register_offset] & (1<<bit_offest));
		
}	
		
		
	

// Define all of the peripheral interrupt callbacks
	



	
/* ============================== GRID_MODULE_INIT() ================================ */


void grid_module_common_init(void){

	grid_ui_model_init(&grid_core_state, 1);
	grid_ui_bank_init(&grid_core_state, 0, 1);
	grid_ui_element_init(&grid_core_state.bank_list[0], 0, GRID_UI_ELEMENT_SYSTEM);
	
		
	if (1){	// INIT CORE_STATE->hearbeat	
		
		uint8_t payload_template[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint8_t payload_length = 0;
	
		sprintf(payload_template, GRID_EVENTSTRING_HEARTBEAT );
		payload_length = strlen(payload_template);
	
		sprintf(&payload_template[payload_length], GRID_CLASS_HEARTBEAT_frame);
		uint8_t error = 0;
		grid_msg_set_parameter(&payload_template[payload_length], GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, &error);
	
		grid_msg_set_parameter(&payload_template[payload_length], GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, grid_sys_get_hwcfg(), &error);
		grid_msg_set_parameter(&payload_template[payload_length], GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length , GRID_PROTOCOL_VERSION_MAJOR, &error);
		grid_msg_set_parameter(&payload_template[payload_length], GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length  , GRID_PROTOCOL_VERSION_MINOR, &error);
		grid_msg_set_parameter(&payload_template[payload_length], GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length  , GRID_PROTOCOL_VERSION_PATCH, &error);
	
		payload_length = strlen(payload_template);
	
		grid_ui_event_register_actionstring(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_HEARTBEAT, payload_template, payload_length);		
		
	}

	if (1){	// INIT CORE_STATE->mapmode press
		
		uint8_t payload_template[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint8_t payload_length = 0;
	
		sprintf(payload_template, GRID_EVENTSTRING_MAPMODE_PRESS GRID_ACTIONSTRING_MAPMODE_PRESS);
		payload_length = strlen(payload_template);
	
		grid_ui_event_register_actionstring(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_MAPMODE_PRESS, payload_template, payload_length);			
		
	}	

	if (1){ // INIT CORE_STATE->mapmode release
			
		uint8_t payload_template[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint8_t payload_length = 0;
		
		sprintf(payload_template, GRID_EVENTSTRING_MAPMODE_RELEASE GRID_ACTIONSTRING_MAPMODE_RELEASE);
		payload_length = strlen(payload_template);
		
		grid_ui_event_register_actionstring(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_MAPMODE_RELEASE, payload_template, payload_length);
		
	}	
	
	if (1){ // INIT CORE_STATE->cfgresponse
		
		uint8_t payload_template[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint8_t payload_length = 0;
		
		sprintf(payload_template, GRID_EVENTSTRING_CFG_RESPONES GRID_ACTIONSTRING_CFG_RESPONSE);
		payload_length = strlen(payload_template);
		
		grid_ui_event_register_actionstring(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_CFG_RESPONSE, payload_template, payload_length);
		
	}	
	
	if (1){ // INIT CORE_STATE->cfgrequest
		
		uint8_t payload_template[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint8_t payload_length = 0;
		
		sprintf(payload_template, GRID_EVENTSTRING_CFG_REQUEST GRID_ACTIONSTRING_CFG_REQUEST);
		payload_length = strlen(payload_template);
		
		grid_ui_event_register_actionstring(&grid_core_state.bank_list[0].element_list[0], GRID_UI_EVENT_CFG_REQUEST, payload_template, payload_length);
		
	}	
	
	
	//enable pwr!
	
	GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "UI Power Enable");
	gpio_set_pin_level(UI_PWR_EN, true);

	// ADC SETUP	
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_PO16_RevB || grid_sys_get_hwcfg() == GRID_MODULE_PO16_RevC){
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: PO16");
		grid_module_po16_revb_init();
	}
	else if (grid_sys_get_hwcfg() == GRID_MODULE_BU16_RevB || grid_sys_get_hwcfg() == GRID_MODULE_BU16_RevC ){
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: BU16");
		grid_module_bu16_revb_init();
	
	}	
	else if (grid_sys_get_hwcfg() == GRID_MODULE_PBF4_RevA){
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: PBF4");					
		grid_module_pbf4_reva_init();			
	}
	else if (grid_sys_get_hwcfg() == GRID_MODULE_EN16_RevA || grid_sys_get_hwcfg() == GRID_MODULE_EN16_RevD ){
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: EN16");
		grid_module_en16_reva_init();	
	}	
	else if (grid_sys_get_hwcfg() == GRID_MODULE_EN16_ND_RevA || grid_sys_get_hwcfg() == GRID_MODULE_EN16_ND_RevD ){
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: EN16 ND");
		grid_module_en16_reva_init();	
	}	
	else{
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "Init Module: Unknown Module");
		GRID_DEBUG_LOG(GRID_DEBUG_CONTEXT_BOOT, "HWCFG Mismatch");
	}


	grid_sys_init(&grid_sys_state);


	grid_nvm_init(&grid_nvm_state, &FLASH_0);
	
		
}

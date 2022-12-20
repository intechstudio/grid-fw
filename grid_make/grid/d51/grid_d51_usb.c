/*
 * grid_d51_usb.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */ 

#include "grid_d51_usb.h"


static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{



	//printf("\r\n$ %d ", count);


	uint8_t halfpacket = 0;
	if (grid_usb_serial_rx_buffer[0] != GRID_CONST_SOH || grid_usb_serial_rx_buffer[count-1] != 10){
		halfpacket = 1;
		//printf("halfpacket");
	}

	for(uint16_t i=0; i<count; i++){

		// if (halfpacket && (i<10 || i>count-10)){
		// 	printf(" %02x", grid_usb_serial_rx_buffer[i]);
		// }
		// else if (halfpacket && i == 10){
		// 	printf(" ...");
		// }

		GRID_PORT_H.rx_double_buffer[grid_usb_rx_double_buffer_index] = grid_usb_serial_rx_buffer[i];

		//printf("%d, ", grid_usb_serial_rx_buffer[i]);
		
		grid_usb_rx_double_buffer_index++;
		grid_usb_rx_double_buffer_index%=GRID_DOUBLE_BUFFER_RX_SIZE;

	}

	// CLEAR THE ENTIRE BUFFER
	for(uint16_t i=0; i<sizeof(grid_usb_serial_rx_buffer); i++){
		grid_usb_serial_rx_buffer[i] = 0;
	}


	cdcdf_acm_read((uint8_t *)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));

	//cdcdf_acm_write(cdcdf_demo_buf, count); /* Echo data */
	return false;                           /* No error. */
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{

	//grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);

	
	return false;                                                                                 /* No error. */
}
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state)
{
	
	//grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	
	//printf("\r\n### USB SERIAL STATE CHANGE %d ###\r\n", sizeof(grid_usb_serial_rx_buffer));

	if (state.rs232.DTR || 1) {
		/* After connection the R/W callbacks can be registered */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_serial_bulkout_cb);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_serial_bulkin_cb);
		/* Start Rx */
		cdcdf_acm_read((uint8_t *)grid_usb_serial_rx_buffer, sizeof(grid_usb_serial_rx_buffer));
	}

	return false; /* No error. */
}



static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{

	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 255);
	return false;
}



void grid_d51_usb_init(void){

	audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkin_cb);


	grid_usb_serial_rx_size = 0;
	grid_usb_serial_rx_flag = 0;
	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)grid_usb_serial_statechange_cb);
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkout_cb);
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);


    
}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3){

    return audiodf_midi_write(byte0, byte1, byte2, byte3);

}


int32_t grid_platform_usb_midi_write_status(void){

    return audiodf_midi_write_status();

}


int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type){

    return hiddf_mouse_button_change(b_state, type);
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis){
 
    return hiddf_mouse_move(position, axis);
}



int32_t grid_platform_usb_keyboard_keys_state_change(void* keys_desc, uint8_t keys_count){

    return hiddf_keyboard_keys_state_change(keys_desc, keys_count);
}
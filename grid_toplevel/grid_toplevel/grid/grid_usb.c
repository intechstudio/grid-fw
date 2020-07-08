/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 */ 

#include "grid_usb.h"
#include "../usb/class/midi/device/audiodf_midi.h"

static bool     grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	//grid_sys_alert_set_alert(&grid_sys_state, 255,255,0,2,300);
//	cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);
	
	//cdcdf_acm_write(cdcdf_demo_buf, count); /* Echo data */
	return false;                           /* No error. */
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	
	//grid_sys_alert_set_alert(&grid_sys_state, 255,0,255,2,300);

//	cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS); /* Another read */
	return false;                                                                                 /* No error. */
}
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state)
{
	
	//grid_sys_alert_set_alert(&grid_sys_state, 0,255,255,2,300);
	
	if (state.rs232.DTR || 1) {
		/* After connection the R/W callbacks can be registered */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_serial_bulkout_cb);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_serial_bulkin_cb);
		/* Start Rx */
		//cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);
	}
	return false; /* No error. */
}
void grid_usb_serial_init()
{
	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)grid_usb_serial_statechange_cb);
}



static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	grid_sys_alert_set_alert(&grid_sys_state, 255,255,0,2,300);
	return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	
	grid_sys_alert_set_alert(&grid_sys_state, 255,0,255,2,300);
	return false;
}




void grid_usb_midi_init()
{
	
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkin_cb);


}
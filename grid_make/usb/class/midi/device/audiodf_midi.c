#include "audiodf_midi.h"

#define AUDIODF_MIDI_VERSION 0x00000001u

/** USB Device Audio Midi Function Specific Data */

struct audiodf_midi_func_data {
	/* AUDIO descriptor */
	uint8_t *audio_desc;

	uint8_t func_iface[2];
	uint8_t func_ep_in;
	uint8_t func_ep_out;
	uint8_t protocol; //????

	uint8_t midi_report[4]; //????

	uint8_t midi_report_inbound[4]; //????

	bool enabled;
};


/* USB Device Audio Midi Function Instance */
static struct usbdf_driver _audiodf_midi;

/* USB Device Audio Midi Function Data Instance */
static struct audiodf_midi_func_data _audiodf_midi_funcd;


volatile uint8_t usb_debug2[10];




// Define the function pointer type
typedef bool (*midi_xfer_cb_t)(const uint8_t, const enum usb_xfer_code, const uint32_t);

// Global function pointer
static midi_xfer_cb_t midi_in_cb = NULL;
static midi_xfer_cb_t midi_out_cb = NULL;
static midi_xfer_cb_t midi_installed_cb = NULL;


/**
 * \brief Enable Audio Midi Function
 * \param[in] drv Pointer to USB device function driver
 * \param[in] desc Pointer to USB interface descriptor
 * \return Operation status.
 */
static int32_t audio_midi_enable(struct usbdf_driver *drv, struct usbd_descriptors *desc)
{

	printf("MIDI ENABLE\n");

	struct audiodf_midi_func_data *func_data = (struct audiodf_midi_func_data *)(drv->func_data);

	usb_iface_desc_t ifc_desc;
	volatile usb_ep_desc_t    ep_desc;
	uint8_t *        ifc, *ep;
	uint8_t          i;

	ifc = desc->sod;

	#define AUDIO_CLASS 0x01		// Audio Class
	#define AUDIO_AC_SUBCLASS 0x01	// Audio Control Subclass
	#define AUDIO_MS_SUBCLASS 0x03	// MidiStreaming Subclass

	for (i=0; i<2; i++){

		if (NULL == ifc) {
			return ERR_NOT_FOUND;
		}

		ifc_desc.bInterfaceNumber = ifc[2];
		ifc_desc.bInterfaceClass  = ifc[5];

		if (AUDIO_AC_SUBCLASS == ifc_desc.bInterfaceClass || AUDIO_MS_SUBCLASS == ifc_desc.bInterfaceClass) {
			if (func_data->func_iface[i] == ifc_desc.bInterfaceNumber) { // Initialized
				return ERR_ALREADY_INITIALIZED;
			} else if (func_data->func_iface[i] != 0xFF) { // Occupied
				return ERR_NO_RESOURCE;
			} else {
				func_data->func_iface[i] = ifc_desc.bInterfaceNumber;
			}
		} else { // Not supported by this function driver
			return ERR_NOT_FOUND;
		}

		//#define USB_DT_AUDIO 0x24
		// Install AUDIO descriptor
		//_audiodf_midi_funcd.audio_desc = usb_find_desc(usb_desc_next(desc->sod), desc->eod, USB_DT_AUDIO);

		// Install endpoints
		if (i == 1){ // i==1 because only the second interface has endpoint descriptors
			ep = usb_find_desc(ifc, desc->eod, USB_DT_ENDPOINT);
			while (NULL != ep) {
				ep_desc.bEndpointAddress = ep[2];
				ep_desc.bmAttributes     = ep[3];
				ep_desc.wMaxPacketSize   = usb_get_u16(ep + 4);

				usb_debug2[4] = ep_desc.wMaxPacketSize;

				usb_debug2[0] = i;

				if (usb_d_ep_init(ep_desc.bEndpointAddress, ep_desc.bmAttributes, ep_desc.wMaxPacketSize)) {
					return ERR_NOT_INITIALIZED;
				}
				if (ep_desc.bEndpointAddress & USB_EP_DIR_IN) {
					func_data->func_ep_in = ep_desc.bEndpointAddress;
					usb_d_ep_enable(func_data->func_ep_in);
				} else {
					func_data->func_ep_out = ep_desc.bEndpointAddress;
					usb_d_ep_enable(func_data->func_ep_out);
				}
				desc->sod = ep;
				ep        = usb_find_ep_desc(usb_desc_next(desc->sod), desc->eod);
			}
		}

		ifc = usb_find_desc(usb_desc_next(desc->sod), desc->eod, USB_DT_INTERFACE);

	}


	printf("MIDI TEST %d %d \r\n", _audiodf_midi_funcd.func_ep_in, _audiodf_midi_funcd.func_ep_out);

	usb_d_ep_register_callback(_audiodf_midi_funcd.func_ep_in, USB_D_EP_CB_XFER, midi_in_cb);
	usb_d_ep_register_callback(_audiodf_midi_funcd.func_ep_out, USB_D_EP_CB_XFER, midi_out_cb);

	if (midi_installed_cb){
		midi_installed_cb(0, 0, 0);
	}


	_audiodf_midi_funcd.enabled = true;
	return ERR_NONE;
}

/**
 * \brief Disable Audio Midi Function
 * \param[in] drv Pointer to USB device function driver
 * \param[in] desc Pointer to USB device descriptor
 * \return Operation status.
 */
static int32_t audio_midi_disable(struct usbdf_driver *drv, struct usbd_descriptors *desc)
{

	struct audiodf_midi_func_data *func_data = (struct audiodf_midi_func_data *)(drv->func_data);

	usb_iface_desc_t ifc_desc;
	uint8_t          i;

	if (desc) {
		ifc_desc.bInterfaceClass = desc->sod[5];
		// Check interface
		if ((AUDIO_AC_SUBCLASS != ifc_desc.bInterfaceClass) && (AUDIO_MS_SUBCLASS != ifc_desc.bInterfaceClass)) {
			return ERR_NOT_FOUND;
		}
	}


	if (func_data->func_iface[0] != 0xFF) {
		func_data->func_iface[0] = 0xFF;
	}


	if (func_data->func_iface[1] != 0xFF) {
		func_data->func_iface[1] = 0xFF;
	}


	if (func_data->func_ep_in != 0xFF) {
		usb_d_ep_deinit(func_data->func_ep_in);
		func_data->func_ep_in = 0xFF;
	}

	if (func_data->func_ep_out != 0xFF) {
		usb_d_ep_deinit(func_data->func_ep_out);
		func_data->func_ep_out = 0xFF;
	}

	_audiodf_midi_funcd.enabled = false;
	return ERR_NONE;

}


static int32_t audio_midi_ctrl(struct usbdf_driver *drv, enum usbdf_control ctrl, void *param)
{

	switch (ctrl) {
		case USBDF_ENABLE:
		return audio_midi_enable(drv, (struct usbd_descriptors *)param);

		case USBDF_DISABLE:
		return audio_midi_disable(drv, (struct usbd_descriptors *)param);

		case USBDF_GET_IFACE:
		return ERR_UNSUPPORTED_OP;

		default:
		return ERR_INVALID_ARG;
	}
}



static int32_t audio_midi_get_desc(uint8_t ep, struct usb_req *req)
{
	return usbdc_xfer(ep, _audiodf_midi_funcd.audio_desc, _audiodf_midi_funcd.audio_desc[0], false);

//	return ERR_INVALID_ARG;

}
/**
 * \brief Process the Audio class request
 * \param[in] ep Endpoint address.
 * \param[in] req Pointer to the request.
 * \return Operation status.
 */

static int32_t audio_midi_req(uint8_t ep, struct usb_req *req, enum usb_ctrl_stage stage)
{

	//return ERR_NOT_FOUND;

	if ((0x81 == req->bmRequestType) && (req->wIndex == _audiodf_midi_funcd.func_iface[0] || req->wIndex == _audiodf_midi_funcd.func_iface[1])) {
		return audio_midi_get_desc(ep, req); // Never hit breakpoint here


	} else {


		if (0x01 != ((req->bmRequestType >> 5) & 0x03)) { // class request
			return ERR_NOT_FOUND; // Never hit breakpoint here
		}
		if (req->wIndex == _audiodf_midi_funcd.func_iface[0] || req->wIndex == _audiodf_midi_funcd.func_iface[1]) {

			// Copied from Hid
			// Never hit breakpoint here
			switch (req->bRequest) {
				case 0x03: /* Get Protocol */
				return usbdc_xfer(ep, &_audiodf_midi_funcd.protocol, 1, 0);
				case 0x0B: /* Set Protocol */
				_audiodf_midi_funcd.protocol = req->wValue;
				return usbdc_xfer(ep, NULL, 0, 0);
				default:
				return ERR_INVALID_ARG;
			}


		} else {
			return ERR_NOT_FOUND;
		}
	}
	(void)stage;
}

/** USB Device Audio Midi Handler Struct */
static struct usbdc_handler audio_midi_req_h = {NULL, (FUNC_PTR)audio_midi_req};

/**
 * \brief Initialize the USB Audio Midi Function Driver
 */
int32_t audiodf_midi_init(void)
{


	if (usbdc_get_state() > USBD_S_POWER) {
		return ERR_DENIED;
	}

	_audiodf_midi.ctrl      = audio_midi_ctrl;
	_audiodf_midi.func_data = &_audiodf_midi_funcd;

	usbdc_register_function(&_audiodf_midi);
	usbdc_register_handler(USBDC_HDL_REQ, &audio_midi_req_h);
	return ERR_NONE;
}

/**
 * \brief Deinitialize the USB Audio Midi Function Driver
 */
int32_t audiodf_midi_deinit(void)
{
	if (usbdc_get_state() > USBD_S_POWER) {
		return ERR_DENIED;
	}

	_audiodf_midi.ctrl      = NULL;
	_audiodf_midi.func_data = NULL;

	usbdc_unregister_function(&_audiodf_midi);
	usbdc_unregister_handler(USBDC_HDL_REQ, &audio_midi_req_h);
	return ERR_NONE;
}

/**
 * \brief Check whether Audio Midi Function is enabled
 */
bool audiodf_midi_is_enabled(void)
{
	return true;
}



int32_t audiodf_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
	if (!audiodf_midi_is_enabled()) {
		return ERR_DENIED;
	}

	// if previous xfer is completed

	_audiodf_midi_funcd.midi_report[0] = byte0;
	_audiodf_midi_funcd.midi_report[1] = byte1;
	_audiodf_midi_funcd.midi_report[2] = byte2;
	_audiodf_midi_funcd.midi_report[3] = byte3;

	return usbdc_xfer(_audiodf_midi_funcd.func_ep_in, _audiodf_midi_funcd.midi_report, 4, false);

}

int32_t audiodf_midi_write_status(){


	struct usb_d_ep_status epstat;
	return usb_d_ep_get_status(_audiodf_midi_funcd.func_ep_in, &epstat);

}



int32_t audiodf_midi_read(uint8_t *buf, uint32_t size)
{
	if (!audiodf_midi_is_enabled()) {
		return ERR_DENIED;
	}

	return usbdc_xfer(_audiodf_midi_funcd.func_ep_out, buf, size, false);
}


int32_t audiodf_midi_register_callback(enum audiodf_midi_cb_type cb_type, FUNC_PTR func)
{
	switch (cb_type) {
		case AUDIODF_MIDI_CB_READ:
		midi_in_cb = func;
		break;
		case AUDIODF_MIDI_CB_WRITE:
		midi_out_cb = func;
		break;
		case AUDIODF_MIDI_CB_INSTALLED:
		midi_installed_cb = func;
		break;
		default:
		return ERR_INVALID_ARG;
	}
	return ERR_NONE;
}



/**
 * \brief Return version
 */
uint32_t audiodf_midi_get_version(void)
{
	return AUDIODF_MIDI_VERSION;
}

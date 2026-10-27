#ifndef GRID_USB_DESC_H
#define GRID_USB_DESC_H

#include "tusb.h"

enum grid_usb_interface {
  ITF_NUM_CDC_NOTIFY = 0,
#if CFG_TUD_CDC
  ITF_NUM_CDC_DATA,
#endif
#if CFG_TUD_MIDI
  ITF_NUM_MIDI,
  ITF_NUM_MIDI_STREAMING,
#endif
#if CFG_TUD_HID
  ITF_NUM_HID,
#endif
  ITF_COUNT,
};

enum grid_usb_endpoint {
  EP_EMPTY = 0,
#if CFG_TUD_CDC
  EPNUM_CDC_NOTIFY,
  EPNUM_CDC_DATA,
#endif
#if CFG_TUD_MIDI
  EPNUM_MIDI,
#endif
#if CFG_TUD_HID
  EPNUM_HID,
#endif
};

#endif /* GRID_USB_DESC_H */

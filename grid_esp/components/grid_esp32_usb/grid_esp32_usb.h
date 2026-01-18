/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// USB class configuration - define before including subsystem headers
#undef CFG_TUD_CDC
#undef CFG_TUD_HID
#undef CFG_TUD_MIDI
#undef CFG_TUD_MSC
#undef CFG_TUD_VENDOR

#define CFG_TUD_CDC 1
#define CFG_TUD_HID 1
#define CFG_TUD_MIDI 1
#define CFG_TUD_MSC 0
#define CFG_TUD_VENDOR 0

#ifdef __cplusplus
}
#endif

// Include all USB subsystem headers (after CFG_TUD_* are defined)
#include "grid_esp32_usb_cdc.h"
#include "grid_esp32_usb_hid.h"
#include "grid_esp32_usb_midi.h"
#include "grid_esp32_usb_ncm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Helper defines **/

void grid_esp32_usb_init(void);

#ifdef __cplusplus
}
#endif

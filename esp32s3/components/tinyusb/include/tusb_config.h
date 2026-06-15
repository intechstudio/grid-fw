#pragma once

// ---- MCU ----
#define CFG_TUSB_MCU OPT_MCU_ESP32S3

// ---- OS abstraction ----
#define CFG_TUSB_OS OPT_OS_NONE

// ---- Debug (0 = silent) ----
#define CFG_TUSB_DEBUG 0

// ---- Root-hub port (full-speed device) — enables no-arg tusb_init() ----
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// ---- Device stack ----
#define CFG_TUD_ENABLED 1

// ---- Endpoint 0 packet size ----
#define CFG_TUD_ENDPOINT0_SIZE 64

// ---- Memory attributes ----
// Plain 4-byte-aligned SRAM (slave mode does not require DMA-capable memory).
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// ============================================================
// Device class drivers
// ============================================================

// CDC (virtual serial) — 1 instance
#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 1024
#define CFG_TUD_CDC_EP_BUFSIZE 512

// MIDI — 1 instance
#ifndef CFG_TUD_MIDI
#define CFG_TUD_MIDI 1
#endif
#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 64
#define CFG_TUD_MIDI_EP_BUFSIZE 64
#define CFG_TUD_MIDI_EPSIZE CFG_TUD_MIDI_EP_BUFSIZE

// HID (keyboard + mouse + gamepad) — 1 instance
#ifndef CFG_TUD_HID
#define CFG_TUD_HID 1
#endif
#define CFG_TUD_HID_EP_BUFSIZE 64

// Disabled classes
#define CFG_TUD_MSC 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_ECM_RNDIS 0
#define CFG_TUD_NCM 0
#define CFG_TUD_AUDIO 0
#define CFG_TUD_VIDEO 0
#define CFG_TUD_DFU 0
#define CFG_TUD_DFU_RUNTIME 0
#define CFG_TUD_BTH 0
#define CFG_TUD_USBTMC 0

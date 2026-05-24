/*
 * TinyUSB configuration for Grid firmware (ESP32-S3).
 *
 * This file is picked up by TinyUSB's tusb_option.h before any TinyUSB
 * macros are defined, so keep it to plain #defines only — no TU_* macros.
 *
 * CFG_TUSB_MCU is injected as a compiler flag from CMakeLists.txt so that
 * a single tusb_config.h works for both ESP32-S2 and ESP32-S3 targets.
 */
#pragma once

// ---- OS abstraction ----
#define CFG_TUSB_OS OPT_OS_FREERTOS

// ---- Debug (0 = silent) ----
#define CFG_TUSB_DEBUG 0

// ---- Device stack ----
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

// Use slave/IRQ mode (no DMA).  DMA requires cache-aligned buffers and
// SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE support; slave mode is simpler and
// fully sufficient for CDC + MIDI + HID at full-speed.
#define CFG_TUD_DWC2_SLAVE_ENABLE 1

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
#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 1024
#define CFG_TUD_CDC_EP_BUFSIZE 512

// MIDI — 1 instance
#define CFG_TUD_MIDI 1
#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 64
#define CFG_TUD_MIDI_EP_BUFSIZE 64
#define CFG_TUD_MIDI_EPSIZE CFG_TUD_MIDI_EP_BUFSIZE

// HID (keyboard + mouse + gamepad) — 1 instance
#define CFG_TUD_HID 1
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

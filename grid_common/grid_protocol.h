/**
 * \file grid_protocol.h
 * \brief This file defines the communictaion classes and protocol constants of
 * Grid.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceded by either a backslash @\
 * or by an at symbol @@.
 *
 * \author Suku Wc (Intech Studio)
 * \see https://intech.studio/
 * \see https://github.com/intechstudio/grid-fw/
 */

#ifndef GRID_PROTOCOL_H
#define GRID_PROTOCOL_H

#define GRID_PROTOCOL_VERSION_MAJOR 1
#define GRID_PROTOCOL_VERSION_MINOR 4
#define GRID_PROTOCOL_VERSION_PATCH 2

// must not change because it would break profiles
#define GRID_PARAMETER_ELEMENT_SYSTEM 0
#define GRID_PARAMETER_ELEMENT_POTMETER 1
#define GRID_PARAMETER_ELEMENT_BUTTON 2
#define GRID_PARAMETER_ELEMENT_ENCODER 3
#define GRID_PARAMETER_ELEMENT_ENDLESS 4
#define GRID_PARAMETER_ELEMENT_LCD 5
#define GRID_PARAMETER_ELEMENT_COUNT 6

// must not change because it would break profiles
#define GRID_PARAMETER_EVENT_INIT 0
#define GRID_PARAMETER_EVENT_POTMETER 1
#define GRID_PARAMETER_EVENT_ENCODER 2
#define GRID_PARAMETER_EVENT_BUTTON 3
#define GRID_PARAMETER_EVENT_MAPMODE 4
#define GRID_PARAMETER_EVENT_MIDIRX 5
#define GRID_PARAMETER_EVENT_TIMER 6
#define GRID_PARAMETER_EVENT_ENDLESS 7
#define GRID_PARAMETER_EVENT_DRAW 8
#define GRID_PARAMETER_EVENT_COUNT 9

// Module HWCFG definitions

#define GRID_MODULE_PO16_RevB 0
#define GRID_MODULE_PO16_RevC 8

// ESP
#define GRID_MODULE_PO16_RevD 1
#define GRID_MODULE_PO16_RevH 3

#define GRID_MODULE_BU16_RevB 128
#define GRID_MODULE_BU16_RevC 136

// ESP
#define GRID_MODULE_BU16_RevD 129
#define GRID_MODULE_BU16_RevH 131

// D51
#define GRID_MODULE_PBF4_RevA 64
// ESP32
#define GRID_MODULE_PBF4_RevD 65
#define GRID_MODULE_PBF4_RevH 67

#define GRID_MODULE_EN16_RevA 192
#define GRID_MODULE_EN16_RevD 193
#define GRID_MODULE_EN16_RevH 195

#define GRID_MODULE_EN16_ND_RevA 200
#define GRID_MODULE_EN16_ND_RevD 201
#define GRID_MODULE_EN16_ND_RevH 203

#define GRID_MODULE_EF44_RevA 32
#define GRID_MODULE_EF44_RevD 33
#define GRID_MODULE_EF44_RevH 35

#define GRID_MODULE_EF44_ND_RevD 41
#define GRID_MODULE_EF44_ND_RevH 43

#define GRID_MODULE_TEK1_RevA 225
#define GRID_MODULE_TEK2_RevA 17

#define GRID_MODULE_VSN1L_RevA 49
#define GRID_MODULE_VSN1R_RevA 81
#define GRID_MODULE_VSN2_RevA 113

#define GRID_MODULE_PB44_RevA 145

#define GRID_MODULE_TEK2_RevB 25
#define GRID_MODULE_VSN1L_RevB 57
#define GRID_MODULE_VSN1R_RevB 89
#define GRID_MODULE_VSN2_RevB 121

#define GRID_MODULE_TEK2_RevH 27
#define GRID_MODULE_VSN1L_RevH 59
#define GRID_MODULE_VSN1R_RevH 91
#define GRID_MODULE_VSN2_RevH 123

#define GRID_MODULE_SOFT_RevA 255

#define GRID_PARAMETER_HEARTBEATINTERVAL_us 250000
#define GRID_PARAMETER_PINGINTERVAL_us 100000

#define GRID_PARAMETER_DISCONNECTTIMEOUT_us 500000

#define GRID_PARAMETER_UICOOLDOWN_us 10000

#define GRID_PARAMETER_DRAWTRIGGER_us 25000

#define GRID_PARAMETER_UART_baudrate 2000000ul

#define GRID_PARAMETER_ACTIONSTRING_maxlength 909

#define GRID_PARAMETER_SPI_TRANSACTION_length 1024
#define GRID_PARAMETER_SPI_SOURCE_FLAGS_index 1012
#define GRID_PARAMETER_SPI_STATUS_FLAGS_index 1013
#define GRID_PARAMETER_SPI_SYNC1_STATE_index 1014
#define GRID_PARAMETER_SPI_SYNC2_STATE_index 1015
#define GRID_PARAMETER_SPI_ROLLING_ID_index 1016
#define GRID_PARAMETER_SPI_BACKLIGHT_PWM_index 1017
#define GRID_PARAMETER_SPI_ROLLING_ID_maximum 64

#define GRID_PARAMETER_PACKET_margin 300

#define GRID_PARAMETER_ELAPSED_LIMIT 10000

#define GRID_PARAMETER_DEFAULT_POSITION 127
#define GRID_PARAMETER_LOCAL_POSITION 255
#define GRID_PARAMETER_GLOBAL_POSITION 0
#define GRID_PARAMETER_DEFAULT_ROTATION 0
#define GRID_PARAMETER_DEFAULT_AGE 0

// SPECIAL CHARACTERS

#define GRID_CONST_NUL 0x00
#define GRID_CONST_SOH 0x01
#define GRID_CONST_STX 0x02
#define GRID_CONST_ETX 0x03
#define GRID_CONST_EOB 0x17
#define GRID_CONST_EOT 0x04

#define GRID_CONST_LF 0x0A

#define GRID_CONST_ACK 0x06
#define GRID_CONST_NAK 0x15
#define GRID_CONST_CAN 0x18

#define GRID_CONST_NORTH 0x11
#define GRID_CONST_EAST 0x12
#define GRID_CONST_SOUTH 0x13
#define GRID_CONST_WEST 0x14

#define GRID_CONST_DCT 0x0E
#define GRID_CONST_BRC 0x0F

#define GRID_CONST_BELL 0x07

#define GRID_PARAMETER_MIDI_NOTEOFF 0x80
#define GRID_PARAMETER_MIDI_NOTEON 0x90
#define GRID_PARAMETER_MIDI_CONTROLCHANGE 0xB0

// HEADER BROADCAST

#define GRID_BRC_frame "%c%c....................%c", GRID_CONST_SOH, GRID_CONST_BRC, GRID_CONST_EOB

#define GRID_BRC_frame_quick "%c%c......%02x7f7f%02x%02x0000%c"

#define GRID_BRC_LEN_offset 2
#define GRID_BRC_LEN_length 4

#define GRID_BRC_ID_offset 6
#define GRID_BRC_ID_length 2

#define GRID_BRC_SESSION_offset 8
#define GRID_BRC_SESSION_length 2

#define GRID_BRC_SX_offset 10
#define GRID_BRC_SX_length 2

#define GRID_BRC_SY_offset 12
#define GRID_BRC_SY_length 2

#define GRID_BRC_DX_offset 14
#define GRID_BRC_DX_length 2

#define GRID_BRC_DY_offset 16
#define GRID_BRC_DY_length 2

#define GRID_BRC_ROT_offset 18
#define GRID_BRC_ROT_length 1

#define GRID_BRC_PORTROT_offset 19
#define GRID_BRC_PORTROT_length 1

#define GRID_BRC_MSGAGE_offset 20
#define GRID_BRC_MSGAGE_length 2

#define GRID_INSTR_length 1
#define GRID_INSTR_offset 4

#define GRID_FOOTER_frame "%c..\n", GRID_CONST_EOT

// Save the following action to the given event & change instruction to execute
#define GRID_INSTR_ACKNOWLEDGE_code 0xA

#define GRID_INSTR_NACKNOWLEDGE_code 0xB

#define GRID_INSTR_CHECK_code 0xC

#define GRID_INSTR_REPORT_code 0xD

#define GRID_INSTR_FETCH_code 0xF

#define GRID_INSTR_EXECUTE_code 0xE

#define GRID_PARAMETER_CLASSCODE_length 3
#define GRID_PARAMETER_CLASSCODE_offset 1

#define GRID_TEMPLATE_UI_PARAMETER_LIST_LENGTH 20

#define GRID_LUA_FNC_G_LED_RED_human "led_default_red"
#define GRID_LUA_FNC_G_LED_RED_fnptr l_led_default_red
#define GRID_LUA_FNC_G_LED_RED_short "glr"

#define GRID_LUA_FNC_G_LED_GRE_human "led_default_green"
#define GRID_LUA_FNC_G_LED_GRE_fnptr l_led_default_green
#define GRID_LUA_FNC_G_LED_GRE_short "glg"

#define GRID_LUA_FNC_G_LED_BLU_human "led_default_blue"
#define GRID_LUA_FNC_G_LED_BLU_fnptr l_led_default_blue
#define GRID_LUA_FNC_G_LED_BLU_short "glb"

#define GRID_LUA_FNC_G_LED_PHASE_short "glp"
#define GRID_LUA_FNC_G_LED_PHASE_human "led_value"
#define GRID_LUA_FNC_G_LED_PHASE_fnptr l_grid_led_layer_phase

#define GRID_LUA_FNC_G_LED_TIMEOUT_short "glt"
#define GRID_LUA_FNC_G_LED_TIMEOUT_human "led_timeout"
#define GRID_LUA_FNC_G_LED_TIMEOUT_fnptr l_grid_led_layer_timeout

#define GRID_LUA_FNC_G_LED_MIN_short "gln"
#define GRID_LUA_FNC_G_LED_MIN_human "led_color_min"
#define GRID_LUA_FNC_G_LED_MIN_fnptr l_grid_led_layer_min

#define GRID_LUA_FNC_G_LED_MID_short "gld"
#define GRID_LUA_FNC_G_LED_MID_human "led_color_mid"
#define GRID_LUA_FNC_G_LED_MID_fnptr l_grid_led_layer_mid

#define GRID_LUA_FNC_G_LED_MAX_short "glx"
#define GRID_LUA_FNC_G_LED_MAX_human "led_color_max"
#define GRID_LUA_FNC_G_LED_MAX_fnptr l_grid_led_layer_max

#define GRID_LUA_FNC_G_LED_COLOR_short "glc"
#define GRID_LUA_FNC_G_LED_COLOR_human "led_color"
#define GRID_LUA_FNC_G_LED_COLOR_fnptr l_grid_led_layer_color

#define GRID_LUA_FNC_G_LED_FREQUENCY_short "glf"
#define GRID_LUA_FNC_G_LED_FREQUENCY_human "led_animation_rate"
#define GRID_LUA_FNC_G_LED_FREQUENCY_fnptr l_grid_led_layer_frequency

#define GRID_LUA_FNC_G_LED_SHAPE_short "gls"
#define GRID_LUA_FNC_G_LED_SHAPE_human "led_animation_type"
#define GRID_LUA_FNC_G_LED_SHAPE_fnptr l_grid_led_layer_shape

#define GRID_LUA_FNC_G_LED_PSF_short "glpfs"
#define GRID_LUA_FNC_G_LED_PSF_human "led_animation_phase_rate_type"
#define GRID_LUA_FNC_G_LED_PSF_fnptr l_grid_led_layer_pfs

#define GRID_LUA_FNC_G_LED_ADDRESS_GET_short "glag"
#define GRID_LUA_FNC_G_LED_ADDRESS_GET_human "led_address_get"
#define GRID_LUA_FNC_G_LED_ADDRESS_GET_fnptr l_grid_led_address_get
#define GRID_LUA_FNC_G_LED_ADDRESS_GET_usage "led_address_get(int element, int n) Returns the hardware index of an element's n-th led, or nil if either parameter is out of bounds."

#define GRID_LUA_FNC_G_MIDI_SEND_short "gms"
#define GRID_LUA_FNC_G_MIDI_SEND_human "midi_send"
#define GRID_LUA_FNC_G_MIDI_SEND_usage "midi_send(int channel, int command, int parameter1, int parameter2) Sends standard MIDI message"
#define GRID_LUA_FNC_G_MIDI_SEND_fnptr l_grid_midi_send

#define GRID_LUA_FNC_G_MIDISYSEX_SEND_short "gmss"
#define GRID_LUA_FNC_G_MIDISYSEX_SEND_human "midi_sysex_send"
#define GRID_LUA_FNC_G_MIDISYSEX_SEND_fnptr l_grid_midi_sysex_send

#define GRID_LUA_FNC_G_KEYBOARD_SEND_short "gks"
#define GRID_LUA_FNC_G_KEYBOARD_SEND_human "keyboard_send"
#define GRID_LUA_FNC_G_KEYBOARD_SEND_fnptr l_grid_usb_keyboard_send

#define GRID_LUA_FNC_G_MOUSEMOVE_SEND_short "gmms"
#define GRID_LUA_FNC_G_MOUSEMOVE_SEND_human "mouse_move_send"
#define GRID_LUA_FNC_G_MOUSEMOVE_SEND_fnptr l_grid_mousemove_send

#define GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short "gmbs"
#define GRID_LUA_FNC_G_MOUSEBUTTON_SEND_human "mouse_button_send"
#define GRID_LUA_FNC_G_MOUSEBUTTON_SEND_fnptr l_grid_mousebutton_send

#define GRID_LUA_FNC_G_GAMEPADMOVE_SEND_short "ggms"
#define GRID_LUA_FNC_G_GAMEPADMOVE_SEND_human "gamepad_move_send"
#define GRID_LUA_FNC_G_GAMEPADMOVE_SEND_fnptr l_grid_gamepadmove_send

#define GRID_LUA_FNC_G_GAMEPADBUTTON_SEND_short "ggbs"
#define GRID_LUA_FNC_G_GAMEPADBUTTON_SEND_human "gamepad_button_send"
#define GRID_LUA_FNC_G_GAMEPADBUTTON_SEND_fnptr l_grid_gamepadbutton_send

#define GRID_LUA_FNC_G_RANDOM_short "grnd"
#define GRID_LUA_FNC_G_RANDOM_human "random8"
#define GRID_LUA_FNC_G_RANDOM_fnptr l_grid_random8

#define GRID_LUA_FNC_G_HWCFG_short "ghwcfg"
#define GRID_LUA_FNC_G_HWCFG_human "hardware_configuration"
#define GRID_LUA_FNC_G_HWCFG_fnptr l_grid_hwcfg

#define GRID_LUA_FNC_G_VERSION_MAJOR_short "gvmaj"
#define GRID_LUA_FNC_G_VERSION_MAJOR_human "version_major"
#define GRID_LUA_FNC_G_VERSION_MAJOR_fnptr l_grid_version_major

#define GRID_LUA_FNC_G_VERSION_MINOR_short "gvmin"
#define GRID_LUA_FNC_G_VERSION_MINOR_human "version_minor"
#define GRID_LUA_FNC_G_VERSION_MINOR_fnptr l_grid_version_minor

#define GRID_LUA_FNC_G_VERSION_PATCH_short "gvpat"
#define GRID_LUA_FNC_G_VERSION_PATCH_human "version_patch"
#define GRID_LUA_FNC_G_VERSION_PATCH_fnptr l_grid_version_patch

#define GRID_LUA_FNC_G_MODULE_POSX_short "gmx"
#define GRID_LUA_FNC_G_MODULE_POSX_human "module_position_x"
#define GRID_LUA_FNC_G_MODULE_POSX_fnptr l_grid_position_x

#define GRID_LUA_FNC_G_MODULE_POSY_short "gmy"
#define GRID_LUA_FNC_G_MODULE_POSY_human "module_position_y"
#define GRID_LUA_FNC_G_MODULE_POSY_fnptr l_grid_position_y

#define GRID_LUA_FNC_G_MODULE_ROT_short "gmr"
#define GRID_LUA_FNC_G_MODULE_ROT_human "module_rotation"
#define GRID_LUA_FNC_G_MODULE_ROT_fnptr l_grid_rotation

#define GRID_LUA_FNC_G_PAGE_NEXT_short "gpn"
#define GRID_LUA_FNC_G_PAGE_NEXT_human "page_next"
#define GRID_LUA_FNC_G_PAGE_NEXT_fnptr l_grid_page_next

#define GRID_LUA_FNC_G_PAGE_PREV_short "gpp"
#define GRID_LUA_FNC_G_PAGE_PREV_human "page_previous"
#define GRID_LUA_FNC_G_PAGE_PREV_fnptr l_grid_page_prev

#define GRID_LUA_FNC_G_PAGE_CURR_short "gpc"
#define GRID_LUA_FNC_G_PAGE_CURR_human "page_current"
#define GRID_LUA_FNC_G_PAGE_CURR_fnptr l_grid_page_curr

#define GRID_LUA_FNC_G_PAGE_LOAD_short "gpl"
#define GRID_LUA_FNC_G_PAGE_LOAD_human "page_load"
#define GRID_LUA_FNC_G_PAGE_LOAD_fnptr l_grid_page_load

#define GRID_LUA_FNC_G_TIMER_START_short "gtt"
#define GRID_LUA_FNC_G_TIMER_START_human "timer_start"
#define GRID_LUA_FNC_G_TIMER_START_fnptr l_grid_timer_start

#define GRID_LUA_FNC_G_TIMER_STOP_short "gtp"
#define GRID_LUA_FNC_G_TIMER_STOP_human "timer_stop"
#define GRID_LUA_FNC_G_TIMER_STOP_fnptr l_grid_timer_stop

#define GRID_LUA_FNC_G_TIMER_SOURCE_short "gts"
#define GRID_LUA_FNC_G_TIMER_SOURCE_human "timer_source"
#define GRID_LUA_FNC_G_TIMER_SOURCE_fnptr l_grid_timer_source

#define GRID_LUA_FNC_G_EVENT_TRIGGER_short "get"
#define GRID_LUA_FNC_G_EVENT_TRIGGER_human "event_trigger"
#define GRID_LUA_FNC_G_EVENT_TRIGGER_fnptr l_grid_event_trigger

#define GRID_LUA_FNC_G_MIDIRX_ENABLED_short "mre"
#define GRID_LUA_FNC_G_MIDIRX_ENABLED_human "midirx_enabled"
#define GRID_LUA_FNC_G_MIDIRX_ENABLED_fnptr l_grid_midirx_enabled

#define GRID_LUA_FNC_G_MIDIRX_SYNC_short "mrs"
#define GRID_LUA_FNC_G_MIDIRX_SYNC_human "midirx_sync"
#define GRID_LUA_FNC_G_MIDIRX_SYNC_fnptr l_grid_midirx_sync

#define GRID_LUA_FNC_G_ELEMENTNAME_SEND_short "gens"
#define GRID_LUA_FNC_G_ELEMENTNAME_SEND_human "element_name_send"
#define GRID_LUA_FNC_G_ELEMENTNAME_SEND_fnptr l_grid_elementname_send

#define GRID_LUA_FNC_G_ELEMENTNAME_SET_short "gsen"
#define GRID_LUA_FNC_G_ELEMENTNAME_SET_human "element_name_set"
#define GRID_LUA_FNC_G_ELEMENTNAME_SET_fnptr l_grid_elementname_set

#define GRID_LUA_FNC_G_ELEMENTNAME_GET_short "ggen"
#define GRID_LUA_FNC_G_ELEMENTNAME_GET_human "element_name_get"
#define GRID_LUA_FNC_G_ELEMENTNAME_GET_fnptr l_grid_elementname_get

#define GRID_LUA_FNC_G_STRING_GET_short "gsg"
#define GRID_LUA_FNC_G_STRING_GET_human "string_get"
#define GRID_LUA_FNC_G_STRING_GET_fnptr l_grid_string_get

#define GRID_LUA_FNC_G_WEBSOCKET_SEND_short "gwss"
#define GRID_LUA_FNC_G_WEBSOCKET_SEND_human "websocket_send"
#define GRID_LUA_FNC_G_WEBSOCKET_SEND_fnptr l_grid_websocket_send

#define GRID_LUA_FNC_G_PACKAGE_SEND_short "gps"
#define GRID_LUA_FNC_G_PACKAGE_SEND_human "package_send"
#define GRID_LUA_FNC_G_PACKAGE_SEND_fnptr l_grid_package_send

#define GRID_LUA_FNC_G_IMMEDIATE_SEND_short "gis"
#define GRID_LUA_FNC_G_IMMEDIATE_SEND_human "immediate_send"
#define GRID_LUA_FNC_G_IMMEDIATE_SEND_fnptr l_grid_immediate_send
#define GRID_LUA_FNC_G_IMMEDIATE_SEND_usage "immediate_send(int x, int y, string lua_code) Executes lua_code on the module addressed by x and y. Use x = nil and y = nil to trigger on all modules."

#define GRID_LUA_FNC_G_FILESYSTEM_LISTDIR_short "gfls"
#define GRID_LUA_FNC_G_FILESYSTEM_LISTDIR_human "readdir"
#define GRID_LUA_FNC_G_FILESYSTEM_LISTDIR_fnptr l_grid_list_dir

#define GRID_LUA_FNC_G_FILESYSTEM_CAT_short "gfcat"
#define GRID_LUA_FNC_G_FILESYSTEM_CAT_human "readfile"
#define GRID_LUA_FNC_G_FILESYSTEM_CAT_fnptr l_grid_cat

#define GRID_LUA_FNC_G_ELEMENT_COUNT_short "gec"
#define GRID_LUA_FNC_G_ELEMENT_COUNT_human "element_count"
#define GRID_LUA_FNC_G_ELEMENT_COUNT_fnptr l_grid_element_count
#define GRID_LUA_FNC_G_ELEMENT_COUNT_usage "element_count(void) Returns the number of elements on the current module."

#define GRID_LUA_FNC_G_CALIBRATION_RESET_short "gcr"
#define GRID_LUA_FNC_G_CALIBRATION_RESET_human "calibration_reset"
#define GRID_LUA_FNC_G_CALIBRATION_RESET_fnptr l_grid_calibration_reset
#define GRID_LUA_FNC_G_CALIBRATION_RESET_usage "calibration_reset() Deletes the calibration file and initializes all calibrations to their defaults."

#define GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_short "gpcg"
#define GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_human "potmeter_calibration_get"
#define GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_fnptr l_grid_potmeter_calibration_get
#define GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_usage "potmeter_calibration_get() Returns raw potentiometer values as an array of integers."

#define GRID_LUA_FNC_G_POTMETER_CENTER_SET_short "gpcs"
#define GRID_LUA_FNC_G_POTMETER_CENTER_SET_human "potmeter_center_set"
#define GRID_LUA_FNC_G_POTMETER_CENTER_SET_fnptr l_grid_potmeter_center_set
#define GRID_LUA_FNC_G_POTMETER_CENTER_SET_usage "potmeter_center_set({ int c1, ... }) Sets potentiometer calibration centers from an array of integers."

#define GRID_LUA_FNC_G_POTMETER_DETENT_SET_short "gpds"
#define GRID_LUA_FNC_G_POTMETER_DETENT_SET_human "potmeter_detent_set"
#define GRID_LUA_FNC_G_POTMETER_DETENT_SET_fnptr l_grid_potmeter_detent_set
#define GRID_LUA_FNC_G_POTMETER_DETENT_SET_usage                                                                                                                                                       \
  "potmeter_detent_set({ int c1, ... }, bool high) Sets potentiometer detent bounds from an array of integers, where high = true sets high bounds and high = false sets low bounds."

#define GRID_LUA_FNC_G_RANGE_CALIBRATION_GET_short "grcg"
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_GET_human "range_calibration_get"
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_GET_fnptr l_grid_range_calibration_get
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_GET_usage "range_calibration_get() Returns raw minimum and maximum values as an array of integer arrays."

#define GRID_LUA_FNC_G_RANGE_CALIBRATION_SET_short "grcs"
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_SET_human "range_calibration_set"
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_SET_fnptr l_grid_range_calibration_set
#define GRID_LUA_FNC_G_RANGE_CALIBRATION_SET_usage "range_calibration_set({ { int min1, int max1 }, ... }) Sets minimum and maximum values from an array of integer arrays."

#define GRID_LUA_FNC_G_GUI_DRAW_SWAP_short "ggdsw"
#define GRID_LUA_FNC_G_GUI_DRAW_SWAP_human "gui_draw_swap"
#define GRID_LUA_FNC_G_GUI_DRAW_SWAP_fnptr l_grid_gui_draw_swap
#define GRID_LUA_FNC_G_GUI_DRAW_SWAP_usage "grid_gui_draw_swap(screen_index) Updates the screen with the contents of the background buffer."

#define GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short "ggdpx"
#define GRID_LUA_FNC_G_GUI_DRAW_PIXEL_human "gui_draw_pixel"
#define GRID_LUA_FNC_G_GUI_DRAW_PIXEL_fnptr l_grid_gui_draw_pixel
#define GRID_LUA_FNC_G_GUI_DRAW_PIXEL_usage "grid_gui_draw_pixel(screen_index, x, y, {r, g, b}) Draws a pixel at (x, y) with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_LINE_short "ggdl"
#define GRID_LUA_FNC_G_GUI_DRAW_LINE_human "gui_draw_line"
#define GRID_LUA_FNC_G_GUI_DRAW_LINE_fnptr l_grid_gui_draw_line
#define GRID_LUA_FNC_G_GUI_DRAW_LINE_usage "grid_gui_draw_line(screen_index, x1, y1, x2, y2, {r, g, b}) Draws a line between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short "ggdr"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_human "gui_draw_rectangle"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_fnptr l_grid_gui_draw_rectangle
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_usage                                                                                                                                                        \
  "grid_gui_draw_rectangle(screen_index, x1, y1, x2, y2, {r, g, b}) Draws a rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short "ggdrf"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_human "gui_draw_rectangle_filled"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_fnptr l_grid_gui_draw_rectangle_filled
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_usage                                                                                                                                                 \
  "grid_gui_draw_rectangle_filled(screen_index, x1, y1, x2, y2, {r, g, b}) Draws a filled rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_short "ggdrr"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_human "gui_draw_rectangle_rounded"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_fnptr l_grid_gui_draw_rectangle_rounded
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_usage                                                                                                                                                \
  "grid_gui_draw_rectangle_rounded(screen_index, x1, y1, x2, y2, radius, {r, g, b}) Draws a rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the specified 8-bit " \
  "color "                                                                                                                                                                                             \
  "channels."

#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_short "ggdrrf"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_human "gui_draw_rectangle_rounded_filled"
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_fnptr l_grid_gui_draw_rectangle_rounded_filled
#define GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_usage                                                                                                                                         \
  "grid_gui_draw_rectangle_rounded_filled(screen_index, x1, y1, x2, y2, radius, {r, g, b}) Draws a filled rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the "   \
  "specified 8-bit "                                                                                                                                                                                   \
  "color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_short "ggdpo"
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_human "gui_draw_polygon"
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_fnptr l_grid_gui_draw_polygon
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_usage                                                                                                                                                          \
  "grid_gui_draw_polygon(screen_index, {x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_short "ggdpof"
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_human "gui_draw_polygon_filled"
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_fnptr l_grid_gui_draw_polygon_filled
#define GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_usage                                                                                                                                                   \
  "grid_gui_draw_polygon_filled(screen_index, {x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a filled polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_TEXT_short "ggdt"
#define GRID_LUA_FNC_G_GUI_DRAW_TEXT_human "gui_draw_text"
#define GRID_LUA_FNC_G_GUI_DRAW_TEXT_fnptr l_grid_gui_draw_text
#define GRID_LUA_FNC_G_GUI_DRAW_TEXT_usage "gui_draw_text(screen_index, 'text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_short "ggdft"
#define GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_human "gui_draw_fasttext"
#define GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_fnptr l_grid_gui_draw_text_fast
#define GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_usage                                                                                                                                                         \
  "gui_draw_fasttext(screen_index, 'text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_short "ggdaf"
#define GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_human "gui_draw_area_filled"
#define GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_fnptr l_grid_gui_draw_area_filled
#define GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_usage "gui_draw_area_filled(screen_index, x1, y1, x2, y2, {r, g, b}) Fills an area with the specified color, without alpha blending."

#define GRID_LUA_FNC_G_GUI_DRAW_DEMO_short "ggdd"
#define GRID_LUA_FNC_G_GUI_DRAW_DEMO_human "gui_draw_demo"
#define GRID_LUA_FNC_G_GUI_DRAW_DEMO_fnptr l_grid_gui_draw_demo
#define GRID_LUA_FNC_G_GUI_DRAW_DEMO_usage "gui_draw_demo(screen_index, n) Draws the n-th iteration of the demo."

#define GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_short "glsb"
#define GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_human "lcd_set_backlight"
#define GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_fnptr l_grid_lcd_set_backlight
#define GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_usage "lcd_set_backlight(strength) Sets the LCD backlight strength between 0 and 255."

// ========================= UI ELEMENT VARIABLES =========================== //

#define GRID_LUA_FNC_B_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_B_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_B_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_B_LED_INDEX_index 1
#define GRID_LUA_FNC_B_LED_INDEX_short "lix"
#define GRID_LUA_FNC_B_LED_INDEX_human "led_index"

#define GRID_LUA_FNC_B_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_B_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_B_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_B_BUTTON_MIN_index 3
#define GRID_LUA_FNC_B_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_B_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_B_BUTTON_MAX_index 4
#define GRID_LUA_FNC_B_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_B_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_B_BUTTON_MODE_index 5
#define GRID_LUA_FNC_B_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_B_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_B_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_B_BUTTON_STATE_index 7
#define GRID_LUA_FNC_B_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_B_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_B_BUTTON_STEP_short "bstp"
#define GRID_LUA_FNC_B_BUTTON_STEP_human "button_step"

#define GRID_LUA_FNC_B_LIST_length 8

#define GRID_LUA_FNC_E_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_E_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_E_LED_INDEX_index 1
#define GRID_LUA_FNC_E_LED_INDEX_short "lix"
#define GRID_LUA_FNC_E_LED_INDEX_human "led_index"

#define GRID_LUA_FNC_E_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_E_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_E_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_E_BUTTON_MIN_index 3
#define GRID_LUA_FNC_E_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_E_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_E_BUTTON_MAX_index 4
#define GRID_LUA_FNC_E_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_E_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_E_BUTTON_MODE_index 5
#define GRID_LUA_FNC_E_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_E_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_E_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_E_BUTTON_STATE_index 7
#define GRID_LUA_FNC_E_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_E_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_E_ENCODER_NUMBER_index 8
#define GRID_LUA_FNC_E_ENCODER_NUMBER_short "enu"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_human "encoder_number"

#define GRID_LUA_FNC_E_ENCODER_VALUE_index 9
#define GRID_LUA_FNC_E_ENCODER_VALUE_short "eva"
#define GRID_LUA_FNC_E_ENCODER_VALUE_human "encoder_value"

#define GRID_LUA_FNC_E_ENCODER_MIN_index 10
#define GRID_LUA_FNC_E_ENCODER_MIN_short "emi"
#define GRID_LUA_FNC_E_ENCODER_MIN_human "encoder_min"

#define GRID_LUA_FNC_E_ENCODER_MAX_index 11
#define GRID_LUA_FNC_E_ENCODER_MAX_short "ema"
#define GRID_LUA_FNC_E_ENCODER_MAX_human "encoder_max"

#define GRID_LUA_FNC_E_ENCODER_MODE_index 12
#define GRID_LUA_FNC_E_ENCODER_MODE_short "emo"
#define GRID_LUA_FNC_E_ENCODER_MODE_human "encoder_mode"

#define GRID_LUA_FNC_E_ENCODER_ELAPSED_index 13
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_short "eel"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_human "encoder_elapsed_time"

#define GRID_LUA_FNC_E_ENCODER_STATE_index 14
#define GRID_LUA_FNC_E_ENCODER_STATE_short "est"
#define GRID_LUA_FNC_E_ENCODER_STATE_human "encoder_state"

#define GRID_LUA_FNC_E_ENCODER_VELOCITY_index 15
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_short "ev0"
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_human "encoder_velocity"

#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index 16
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_short "ese"
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_human "encoder_sensitivity"

#define GRID_LUA_FNC_E_BUTTON_STEP_short "bstp"
#define GRID_LUA_FNC_E_BUTTON_STEP_human "button_step"

#define GRID_LUA_FNC_E_LED_COLOR_short "glc"
#define GRID_LUA_FNC_E_LED_COLOR_human "led_color"

#define GRID_LUA_FNC_E_LIST_length 17

#define GRID_LUA_FNC_EP_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_EP_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_EP_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_EP_LED_INDEX_index 1
#define GRID_LUA_FNC_EP_LED_INDEX_short "lix"
#define GRID_LUA_FNC_EP_LED_INDEX_human "led_index"

#define GRID_LUA_FNC_EP_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_EP_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_EP_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_EP_BUTTON_MIN_index 3
#define GRID_LUA_FNC_EP_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_EP_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_EP_BUTTON_MAX_index 4
#define GRID_LUA_FNC_EP_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_EP_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_EP_BUTTON_MODE_index 5
#define GRID_LUA_FNC_EP_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_EP_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_EP_BUTTON_STATE_index 7
#define GRID_LUA_FNC_EP_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_EP_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_EP_LED_OFFSET_index 8
#define GRID_LUA_FNC_EP_LED_OFFSET_short "lof"
#define GRID_LUA_FNC_EP_LED_OFFSET_human "led_offset"

#define GRID_LUA_FNC_EP_ENDLESS_VALUE_index 9
#define GRID_LUA_FNC_EP_ENDLESS_VALUE_short "epva"
#define GRID_LUA_FNC_EP_ENDLESS_VALUE_human "endless_value"

#define GRID_LUA_FNC_EP_ENDLESS_MIN_index 10
#define GRID_LUA_FNC_EP_ENDLESS_MIN_short "epmi"
#define GRID_LUA_FNC_EP_ENDLESS_MIN_human "endless_min"

#define GRID_LUA_FNC_EP_ENDLESS_MAX_index 11
#define GRID_LUA_FNC_EP_ENDLESS_MAX_short "epma"
#define GRID_LUA_FNC_EP_ENDLESS_MAX_human "endless_max"

#define GRID_LUA_FNC_EP_ENDLESS_MODE_index 12
#define GRID_LUA_FNC_EP_ENDLESS_MODE_short "epmo"
#define GRID_LUA_FNC_EP_ENDLESS_MODE_human "endless_mode"

#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index 13
#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_short "epel"
#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_human "endless_elapsed_time"

#define GRID_LUA_FNC_EP_ENDLESS_STATE_index 14
#define GRID_LUA_FNC_EP_ENDLESS_STATE_short "epst"
#define GRID_LUA_FNC_EP_ENDLESS_STATE_human "endless_state"

#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index 15
#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_short "epv0"
#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_human "endless_velocity"

#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index 16
#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_short "epdir"
#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_human "endless_direction"

#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index 17
#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_short "epse"
#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_human "endless_sensitivity"

#define GRID_LUA_FNC_EP_LIST_length 18

#define GRID_LUA_FNC_L_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_L_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_L_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_L_SCREEN_INDEX_index 1
#define GRID_LUA_FNC_L_SCREEN_INDEX_short "lin"
#define GRID_LUA_FNC_L_SCREEN_INDEX_human "screen_index"
#define GRID_LUA_FNC_L_SCREEN_INDEX_usage "lcd:screen_index() Returns the screen index used by low-level APIs."

#define GRID_LUA_FNC_L_SCREEN_WIDTH_index 2
#define GRID_LUA_FNC_L_SCREEN_WIDTH_short "lsw"
#define GRID_LUA_FNC_L_SCREEN_WIDTH_human "screen_width"
#define GRID_LUA_FNC_L_SCREEN_WIDTH_usage "lcd:screen_width() Returns the screen width in pixels."

#define GRID_LUA_FNC_L_SCREEN_HEIGHT_index 3
#define GRID_LUA_FNC_L_SCREEN_HEIGHT_short "lsh"
#define GRID_LUA_FNC_L_SCREEN_HEIGHT_human "screen_height"
#define GRID_LUA_FNC_L_SCREEN_HEIGHT_usage "lcd:screen_height() Returns the screen height in pixels."

#define GRID_LUA_FNC_L_LIST_length 4

#define GRID_LUA_FNC_P_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_P_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_P_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_P_LED_INDEX_index 1
#define GRID_LUA_FNC_P_LED_INDEX_short "lix"
#define GRID_LUA_FNC_P_LED_INDEX_human "led_index"

#define GRID_LUA_FNC_P_POTMETER_VALUE_index 2
#define GRID_LUA_FNC_P_POTMETER_VALUE_short "pva"
#define GRID_LUA_FNC_P_POTMETER_VALUE_human "potmeter_value"

#define GRID_LUA_FNC_P_POTMETER_MIN_index 3
#define GRID_LUA_FNC_P_POTMETER_MIN_short "pmi"
#define GRID_LUA_FNC_P_POTMETER_MIN_human "potmeter_min"

#define GRID_LUA_FNC_P_POTMETER_MAX_index 4
#define GRID_LUA_FNC_P_POTMETER_MAX_short "pma"
#define GRID_LUA_FNC_P_POTMETER_MAX_human "potmeter_max"

#define GRID_LUA_FNC_P_POTMETER_MODE_index 5
#define GRID_LUA_FNC_P_POTMETER_MODE_short "pmo"
#define GRID_LUA_FNC_P_POTMETER_MODE_human "potmeter_resolution"

#define GRID_LUA_FNC_P_POTMETER_ELAPSED_index 6
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_short "pel"
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_human "potmeter_elapsed_time"

#define GRID_LUA_FNC_P_POTMETER_STATE_index 7
#define GRID_LUA_FNC_P_POTMETER_STATE_short "pst"
#define GRID_LUA_FNC_P_POTMETER_STATE_human "potmeter_state"

#define GRID_LUA_FNC_P_LIST_length 8

#define GRID_LUA_FNC_S_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_S_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_S_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_S_LIST_length 1

// ========================= UI EVENT HANDLER FUNCTIONS =========================== //

#define GRID_LUA_FNC_A_INIT_short "ini"
#define GRID_LUA_FNC_A_INIT_human "init_handler"

#define GRID_LUA_FNC_A_ENCODER_short "ec"
#define GRID_LUA_FNC_A_ENCODER_human "encoder_handler"

#define GRID_LUA_FNC_A_BUTTON_short "bc"
#define GRID_LUA_FNC_A_BUTTON_human "button_handler"

#define GRID_LUA_FNC_A_POTMETER_short "pc"
#define GRID_LUA_FNC_A_POTMETER_human "potmeter_handler"

#define GRID_LUA_FNC_A_TIMER_short "tim"
#define GRID_LUA_FNC_A_TIMER_human "timer_handler"

#define GRID_LUA_FNC_A_MAPMODE_short "map"
#define GRID_LUA_FNC_A_MAPMODE_human "mapmode_handler"

#define GRID_LUA_FNC_A_MIDIRX_short "mrx"
#define GRID_LUA_FNC_A_MIDIRX_human "midirx"

#define GRID_LUA_FNC_A_ENDLESS_short "epc"
#define GRID_LUA_FNC_A_ENDLESS_human "endless_handler"

#define GRID_LUA_FNC_A_DRAW_short "ld"
#define GRID_LUA_FNC_A_DRAW_human "draw_handler"

#define GRID_LUA_KW_ELEMENT_short "ele"
#define GRID_LUA_KW_ELEMENT_human "element"

// ================== MIDI CLASS =================== //
#define GRID_CLASS_MIDI_code 0x000
#define GRID_CLASS_MIDI_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_MIDI_code, GRID_CONST_ETX

#define GRID_CLASS_MIDI_CHANNEL_offset 5
#define GRID_CLASS_MIDI_CHANNEL_length 2

#define GRID_CLASS_MIDI_COMMAND_offset 7
#define GRID_CLASS_MIDI_COMMAND_length 2

#define GRID_CLASS_MIDI_PARAM1_offset 9
#define GRID_CLASS_MIDI_PARAM1_length 2

#define GRID_CLASS_MIDI_PARAM2_offset 11
#define GRID_CLASS_MIDI_PARAM2_length 2

#define GRID_CLASS_MIDISYSEX_code 0x001
#define GRID_CLASS_MIDISYSEX_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_MIDISYSEX_code
#define GRID_CLASS_MIDISYSEX_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_MIDISYSEX_LENGTH_offset 5
#define GRID_CLASS_MIDISYSEX_LENGTH_length 4

#define GRID_CLASS_MIDISYSEX_PAYLOAD_offset 9
#define GRID_CLASS_MIDISYSEX_PAYLOAD_length 2

// HEARTBEAT (type=0 grid, type=1 gridmaster, type=255 editor)
#define GRID_CLASS_HEARTBEAT_code 0x010
#define GRID_CLASS_HEARTBEAT_frame "%c%03x_..............%c", GRID_CONST_STX, GRID_CLASS_HEARTBEAT_code, GRID_CONST_ETX

#define GRID_CLASS_HEARTBEAT_TYPE_offset 5
#define GRID_CLASS_HEARTBEAT_TYPE_length 2

#define GRID_CLASS_HEARTBEAT_HWCFG_offset 7
#define GRID_CLASS_HEARTBEAT_HWCFG_length 2

#define GRID_CLASS_HEARTBEAT_VMAJOR_offset 9
#define GRID_CLASS_HEARTBEAT_VMAJOR_length 2

#define GRID_CLASS_HEARTBEAT_VMINOR_offset 11
#define GRID_CLASS_HEARTBEAT_VMINOR_length 2

#define GRID_CLASS_HEARTBEAT_VPATCH_offset 13
#define GRID_CLASS_HEARTBEAT_VPATCH_length 2

#define GRID_CLASS_HEARTBEAT_PORTSTATE_offset 15
#define GRID_CLASS_HEARTBEAT_PORTSTATE_length 2

#define GRID_CLASS_HEARTBEAT_GCCOUNT_offset 17
#define GRID_CLASS_HEARTBEAT_GCCOUNT_length 2

// SERIAL NUMBER
#define GRID_CLASS_SERIALNUMBER_code 0x011
#define GRID_CLASS_SERIALNUMBER_frame "%c%03x_................................%c", GRID_CONST_STX, GRID_CLASS_SERIALNUMBER_code, GRID_CONST_ETX

#define GRID_CLASS_SERIALNUMBER_WORD0_offset 5
#define GRID_CLASS_SERIALNUMBER_WORD0_length 8

#define GRID_CLASS_SERIALNUMBER_WORD1_offset 13
#define GRID_CLASS_SERIALNUMBER_WORD1_length 8

#define GRID_CLASS_SERIALNUMBER_WORD2_offset 21
#define GRID_CLASS_SERIALNUMBER_WORD2_length 8

#define GRID_CLASS_SERIALNUMBER_WORD3_offset 29
#define GRID_CLASS_SERIALNUMBER_WORD3_length 8

// RESETCAUSE
#define GRID_CLASS_RESETCAUSE_code 0x012
#define GRID_CLASS_RESETCAUSE_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_RESETCAUSE_code, GRID_CONST_ETX

#define GRID_CLASS_RESETCAUSE_CAUSE_offset 5
#define GRID_CLASS_RESETCAUSE_CAUSE_length 2

// RESET
#define GRID_CLASS_RESET_code 0x013
#define GRID_CLASS_RESET_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_RESET_code, GRID_CONST_ETX

// UPTIME
#define GRID_CLASS_UPTIME_code 0x014
#define GRID_CLASS_UPTIME_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_UPTIME_code, GRID_CONST_ETX

#define GRID_CLASS_UPTIME_UPTIME_offset 5
#define GRID_CLASS_UPTIME_UPTIME_length 16

// DEBUGTEXT
#define GRID_CLASS_DEBUGTEXT_code 0x020
#define GRID_CLASS_DEBUGTEXT_frame_start "%c%03x_", GRID_CONST_STX, GRID_CLASS_DEBUGTEXT_code
#define GRID_CLASS_DEBUGTEXT_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_DEBUGTEXT_TEXT_offset 5
#define GRID_CLASS_DEBUGTEXT_TEXT_length 0

// DEBUGTASK
#define GRID_CLASS_DEBUGTASK_code 0x021
#define GRID_CLASS_DEBUGTASK_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_DEBUGTASK_code
#define GRID_CLASS_DEBUGTASK_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_DEBUGTASK_LENGTH_offset 5
#define GRID_CLASS_DEBUGTASK_LENGTH_length 4

#define GRID_CLASS_DEBUGTASK_OUTPUT_offset 9
#define GRID_CLASS_DEBUGTASK_OUTPUT_length 0

// WEBSOCKET
#define GRID_CLASS_WEBSOCKET_code 0x022
#define GRID_CLASS_WEBSOCKET_frame_start "%c%03x_", GRID_CONST_STX, GRID_CLASS_WEBSOCKET_code
#define GRID_CLASS_WEBSOCKET_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_WEBSOCKET_TEXT_offset 5
#define GRID_CLASS_WEBSOCKET_TEXT_length 0

// PACKAGE
#define GRID_CLASS_PACKAGE_code 0x023
#define GRID_CLASS_PACKAGE_frame_start "%c%03x_", GRID_CONST_STX, GRID_CLASS_PACKAGE_code
#define GRID_CLASS_PACKAGE_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_PACKAGE_TEXT_offset 5
#define GRID_CLASS_PACKAGE_TEXT_length 0

// PAGEACTIVE
#define GRID_CLASS_PAGEACTIVE_code 0x030
#define GRID_CLASS_PAGEACTIVE_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_PAGEACTIVE_code, GRID_CONST_ETX

#define GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset 5
#define GRID_CLASS_PAGEACTIVE_PAGENUMBER_length 2

// PAGECOUNT
#define GRID_CLASS_PAGECOUNT_code 0x031
#define GRID_CLASS_PAGECOUNT_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_PAGECOUNT_code, GRID_CONST_ETX

#define GRID_CLASS_PAGECOUNT_PAGENUMBER_offset 5
#define GRID_CLASS_PAGECOUNT_PAGENUMBER_length 2

#define GRID_CLASS_LEDPREVIEW_code 0x042
#define GRID_CLASS_LEDPREVIEW_frame "%c%03x_............%c", GRID_CONST_STX, GRID_CLASS_LEDPREVIEW_code, GRID_CONST_ETX
#define GRID_CLASS_LEDPREVIEW_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_LEDPREVIEW_code
#define GRID_CLASS_LEDPREVIEW_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_LEDPREVIEW_LENGTH_offset 5
#define GRID_CLASS_LEDPREVIEW_LENGTH_length 4

#define GRID_CLASS_LEDPREVIEW_NUM_offset 9
#define GRID_CLASS_LEDPREVIEW_NUM_length 2

#define GRID_CLASS_LEDPREVIEW_RED_offset 11
#define GRID_CLASS_LEDPREVIEW_RED_length 2

#define GRID_CLASS_LEDPREVIEW_GRE_offset 13
#define GRID_CLASS_LEDPREVIEW_GRE_length 2

#define GRID_CLASS_LEDPREVIEW_BLU_offset 15
#define GRID_CLASS_LEDPREVIEW_BLU_length 2

#define GRID_CLASS_EVENT_code 0x050
#define GRID_CLASS_EVENT_frame "%c%03x_..........%c", GRID_CONST_STX, GRID_CLASS_EVENT_code, GRID_CONST_ETX

#define GRID_CLASS_EVENT_PAGENUMBER_offset 5
#define GRID_CLASS_EVENT_PAGENUMBER_length 2

#define GRID_CLASS_EVENT_ELEMENTNUMBER_offset 7
#define GRID_CLASS_EVENT_ELEMENTNUMBER_length 2

#define GRID_CLASS_EVENT_EVENTTYPE_offset 9
#define GRID_CLASS_EVENT_EVENTTYPE_length 2

#define GRID_CLASS_EVENT_EVENTPARAM1_offset 11
#define GRID_CLASS_EVENT_EVENTPARAM1_length 2

#define GRID_CLASS_EVENT_EVENTPARAM2_offset 13
#define GRID_CLASS_EVENT_EVENTPARAM2_length 2

#define GRID_CLASS_EVENTPREVIEW_code 0x051
#define GRID_CLASS_EVENTPREVIEW_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_EVENTPREVIEW_code, GRID_CONST_ETX
#define GRID_CLASS_EVENTPREVIEW_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_EVENTPREVIEW_code
#define GRID_CLASS_EVENTPREVIEW_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_EVENTPREVIEW_LENGTH_offset 5
#define GRID_CLASS_EVENTPREVIEW_LENGTH_length 4

#define GRID_CLASS_EVENTPREVIEW_NUM_offset 9
#define GRID_CLASS_EVENTPREVIEW_NUM_length 2

#define GRID_CLASS_EVENTPREVIEW_VALUE_offset 11
#define GRID_CLASS_EVENTPREVIEW_VALUE_length 2

// STRNGNAME
#define GRID_CLASS_ELEMENTNAME_code 0x052
#define GRID_CLASS_ELEMENTNAME_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_ELEMENTNAME_code, GRID_CONST_ETX
#define GRID_CLASS_ELEMENTNAME_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_ELEMENTNAME_code
#define GRID_CLASS_ELEMENTNAME_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_ELEMENTNAME_NUM_offset 5
#define GRID_CLASS_ELEMENTNAME_NUM_length 2

#define GRID_CLASS_ELEMENTNAME_LENGTH_offset 7
#define GRID_CLASS_ELEMENTNAME_LENGTH_length 2

#define GRID_CLASS_ELEMENTNAME_NAME_offset 9
#define GRID_CLASS_ELEMENTNAME_NAME_length 0

// EVENT VIEW
#define GRID_CLASS_EVENTVIEW_code 0x053
#define GRID_CLASS_EVENTVIEW_frame_start "%c%03x_..................", GRID_CONST_STX, GRID_CLASS_EVENTVIEW_code
#define GRID_CLASS_EVENTVIEW_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_EVENTVIEW_PAGE_offset 5
#define GRID_CLASS_EVENTVIEW_PAGE_length 2

#define GRID_CLASS_EVENTVIEW_ELEMENT_offset 7
#define GRID_CLASS_EVENTVIEW_ELEMENT_length 2

#define GRID_CLASS_EVENTVIEW_EVENT_offset 9
#define GRID_CLASS_EVENTVIEW_EVENT_length 2

#define GRID_CLASS_EVENTVIEW_VALUE1_offset 11
#define GRID_CLASS_EVENTVIEW_VALUE1_length 4

#define GRID_CLASS_EVENTVIEW_MIN1_offset 15
#define GRID_CLASS_EVENTVIEW_MIN1_length 4

#define GRID_CLASS_EVENTVIEW_MAX1_offset 19
#define GRID_CLASS_EVENTVIEW_MAX1_length 4

#define GRID_CLASS_EVENTVIEW_LENGTH_offset 23
#define GRID_CLASS_EVENTVIEW_LENGTH_length 2

#define GRID_CLASS_EVENTVIEW_NAME_offset 25
#define GRID_CLASS_EVENTVIEW_NAME_length 0

// NAME PREVIEW
#define GRID_CLASS_NAMEPREVIEW_code 0x054

// CONFIG STORE     Fetch(Read) Configure(Overwrite) Append(Write)
#define GRID_CLASS_CONFIG_code 0x060
#define GRID_CLASS_CONFIG_frame "%c%03x_................%c", GRID_CONST_STX, GRID_CLASS_CONFIG_code, GRID_CONST_ETX
#define GRID_CLASS_CONFIG_frame_start "%c%03x_................", GRID_CONST_STX, GRID_CLASS_CONFIG_code
#define GRID_CLASS_CONFIG_frame_end "%c", GRID_CONST_ETX

// used when check instruction is received
#define GRID_CLASS_CONFIG_frame_check "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_CONFIG_code, GRID_CONST_ETX

#define GRID_CLASS_CONFIG_LASTHEADER_offset 5
#define GRID_CLASS_CONFIG_LASTHEADER_length 2

#define GRID_CLASS_CONFIG_VERSIONMAJOR_offset 5
#define GRID_CLASS_CONFIG_VERSIONMAJOR_length 2

#define GRID_CLASS_CONFIG_VERSIONMINOR_offset 7
#define GRID_CLASS_CONFIG_VERSIONMINOR_length 2

#define GRID_CLASS_CONFIG_VERSIONPATCH_offset 9
#define GRID_CLASS_CONFIG_VERSIONPATCH_length 2

#define GRID_CLASS_CONFIG_PAGENUMBER_offset 11
#define GRID_CLASS_CONFIG_PAGENUMBER_length 2

#define GRID_CLASS_CONFIG_ELEMENTNUMBER_offset 13
#define GRID_CLASS_CONFIG_ELEMENTNUMBER_length 2

#define GRID_CLASS_CONFIG_EVENTTYPE_offset 15
#define GRID_CLASS_CONFIG_EVENTTYPE_length 2

#define GRID_CLASS_CONFIG_ACTIONLENGTH_offset 17
#define GRID_CLASS_CONFIG_ACTIONLENGTH_length 4

#define GRID_CLASS_CONFIG_ACTIONSTRING_offset 21
#define GRID_CLASS_CONFIG_ACTIONSTRING_length 0

// =========== PAGE STORE =========== //
#define GRID_CLASS_PAGESTORE_code 0x061
#define GRID_CLASS_PAGESTORE_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_PAGESTORE_code, GRID_CONST_ETX
// used when check instruction is received
#define GRID_CLASS_PAGESTORE_LASTHEADER_offset 5
#define GRID_CLASS_PAGESTORE_LASTHEADER_length 2

// =========== NVM ERASE =========== //
#define GRID_CLASS_NVMERASE_code 0x062
#define GRID_CLASS_NVMERASE_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_NVMERASE_code, GRID_CONST_ETX
// used when check instruction is received
#define GRID_CLASS_NVMERASE_LASTHEADER_offset 5
#define GRID_CLASS_NVMERASE_LASTHEADER_length 2

// =========== PAGE DISCARD =========== //
#define GRID_CLASS_PAGEDISCARD_code 0x063
#define GRID_CLASS_PAGEDISCARD_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_PAGEDISCARD_code, GRID_CONST_ETX
// used when check instruction is received
#define GRID_CLASS_PAGEDISCARD_LASTHEADER_offset 5
#define GRID_CLASS_PAGEDISCARD_LASTHEADER_length 2

// =========== PAGE CLEAR =========== //
#define GRID_CLASS_PAGECLEAR_code 0x064
#define GRID_CLASS_PAGECLEAR_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_PAGECLEAR_code, GRID_CONST_ETX
// used when check instruction is received
#define GRID_CLASS_PAGECLEAR_LASTHEADER_offset 5
#define GRID_CLASS_PAGECLEAR_LASTHEADER_length 2

// =========== NVM DEFRAG =========== //
#define GRID_CLASS_NVMDEFRAG_code 0x065
#define GRID_CLASS_NVMDEFRAG_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_NVMDEFRAG_code, GRID_CONST_ETX
// used when check instruction is received
#define GRID_CLASS_NVMDEFRAG_LASTHEADER_offset 5
#define GRID_CLASS_NVMDEFRAG_LASTHEADER_length 2

// RUN immediate
#define GRID_CLASS_IMMEDIATE_code 0x085
#define GRID_CLASS_IMMEDIATE_frame_start "%c%03x_....", GRID_CONST_STX, GRID_CLASS_IMMEDIATE_code
#define GRID_CLASS_IMMEDIATE_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_IMMEDIATE_ACTIONLENGTH_offset 5
#define GRID_CLASS_IMMEDIATE_ACTIONLENGTH_length 4

#define GRID_CLASS_IMMEDIATE_ACTIONSTRING_offset 9
#define GRID_CLASS_IMMEDIATE_ACTIONSTRING_length 0

// 090 HID KEYBOARD STATUS

#define GRID_CLASS_HIDKEYSTATUS_code 0x090
#define GRID_CLASS_HIDKEYSTATUS_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_HIDKEYSTATUS_code, GRID_CONST_ETX

#define GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset 5
#define GRID_CLASS_HIDKEYSTATUS_ISENABLED_length 2

// 091 HID KEYBOARD LOWLEVEL KEYPRESS/RELEASE

#define GRID_CLASS_HIDKEYBOARD_code 0x091
#define GRID_CLASS_HIDKEYBOARD_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_HIDKEYBOARD_code, GRID_CONST_ETX
#define GRID_CLASS_HIDKEYBOARD_frame_start "%c%03x_........", GRID_CONST_STX, GRID_CLASS_HIDKEYBOARD_code
#define GRID_CLASS_HIDKEYBOARD_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset 5
#define GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length 2

#define GRID_CLASS_HIDKEYBOARD_LENGTH_offset 7
#define GRID_CLASS_HIDKEYBOARD_LENGTH_length 2

#define GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset 9
#define GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length 1

#define GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset 10
#define GRID_CLASS_HIDKEYBOARD_KEYSTATE_length 1

#define GRID_CLASS_HIDKEYBOARD_KEYCODE_offset 11
#define GRID_CLASS_HIDKEYBOARD_KEYCODE_length 2

#define GRID_CLASS_HIDKEYBOARD_DELAY_offset 10
#define GRID_CLASS_HIDKEYBOARD_DELAY_length 3

// 092 HID MOUSE MOVE (X, Y or WHEEL)
#define GRID_CLASS_HIDMOUSEMOVE_code 0x092
#define GRID_CLASS_HIDMOUSEMOVE_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_HIDMOUSEMOVE_code, GRID_CONST_ETX

#define GRID_CLASS_HIDMOUSEMOVE_AXIS_offset 5
#define GRID_CLASS_HIDMOUSEMOVE_AXIS_length 2

#define GRID_CLASS_HIDMOUSEMOVE_POSITION_offset 7
#define GRID_CLASS_HIDMOUSEMOVE_POSITION_length 2

// 093 HID MOUSE BUTTON (left, right, middle)
#define GRID_CLASS_HIDMOUSEBUTTON_code 0x093
#define GRID_CLASS_HIDMOUSEBUTTON_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_HIDMOUSEBUTTON_code, GRID_CONST_ETX

#define GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset 5
#define GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length 2

#define GRID_CLASS_HIDMOUSEBUTTON_STATE_offset 7
#define GRID_CLASS_HIDMOUSEBUTTON_STATE_length 2

// 094 HID GAMEPAD MOVE (X, Y ...)
#define GRID_CLASS_HIDGAMEPADMOVE_code 0x094
#define GRID_CLASS_HIDGAMEPADMOVE_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_HIDGAMEPADMOVE_code, GRID_CONST_ETX

#define GRID_CLASS_HIDGAMEPADMOVE_AXIS_offset 5
#define GRID_CLASS_HIDGAMEPADMOVE_AXIS_length 2

#define GRID_CLASS_HIDGAMEPADMOVE_POSITION_offset 7
#define GRID_CLASS_HIDGAMEPADMOVE_POSITION_length 2

// 095 HID GAMEPAD BUTTON (0, 1 ... 31)
#define GRID_CLASS_HIDGAMEPADBUTTON_code 0x095
#define GRID_CLASS_HIDGAMEPADBUTTON_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_HIDGAMEPADBUTTON_code, GRID_CONST_ETX

#define GRID_CLASS_HIDGAMEPADBUTTON_BUTTON_offset 5
#define GRID_CLASS_HIDGAMEPADBUTTON_BUTTON_length 2

#define GRID_CLASS_HIDGAMEPADBUTTON_STATE_offset 7
#define GRID_CLASS_HIDGAMEPADBUTTON_STATE_length 2

#endif /* GRID_PROTOCOL_H */

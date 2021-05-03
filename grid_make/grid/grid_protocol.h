#ifndef GRID_PROTOCOL_H_INCLUDED
#define GRID_PROTOCOL_H_INCLUDED

#define GRID_PROTOCOL_VERSION_MAJOR 1
#define GRID_PROTOCOL_VERSION_MINOR 1
#define GRID_PROTOCOL_VERSION_PATCH 9

// Module HWCFG definitions

#define GRID_MODULE_PO16_RevB 0
#define GRID_MODULE_PO16_RevC 8

#define GRID_MODULE_BU16_RevB 128
#define GRID_MODULE_BU16_RevC 136

#define GRID_MODULE_PBF4_RevA 64

#define GRID_MODULE_EN16_RevA 192
#define GRID_MODULE_EN16_RevD 193

#define GRID_MODULE_EN16_ND_RevA 200
#define GRID_MODULE_EN16_ND_RevD 201

#define GRID_PARAMETER_HEARTBEAT_interval 250
#define GRID_PARAMETER_PING_interval 100

#define GRID_PARAMETER_TEXT_maxlength 150
#define GRID_PARAMETER_PACKET_maxlength 400
#define GRID_PARAMETER_PACKET_marign 200

// SPECIAL CHARACTERS

#define GRID_CONST_NUL 0x00

#define GRID_CONST_SOH 0x01 //start of header
#define GRID_CONST_STX 0x02 //start of text
#define GRID_CONST_ETX 0x03 //end of text
#define GRID_CONST_EOB 0x17 //end of block
#define GRID_CONST_EOT 0x04 //end of transmission

#define GRID_CONST_LF 0x0A //linefeed, newline

#define GRID_CONST_ACK 0x06 //acknowledge
#define GRID_CONST_NAK 0x15 //nacknowledge
#define GRID_CONST_CAN 0x18 //cancel

#define GRID_CONST_NORTH 0x11 // Device Control 1
#define GRID_CONST_EAST 0x12  // Device Control 2
#define GRID_CONST_SOUTH 0x13 // Device Control 3
#define GRID_CONST_WEST 0x14  // Device Control 4

#define GRID_CONST_DCT 0x0E // Shift In = Direct
#define GRID_CONST_BRC 0x0F // Shift Out = Broadcast

#define GRID_CONST_BELL 0x07

#define GRID_PARAMETER_MIDI_NOTEOFF 0x80
#define GRID_PARAMETER_MIDI_NOTEON 0x90
#define GRID_PARAMETER_MIDI_CONTROLCHANGE 0xB0

#define GRID_PARAMETER_KEYBOARD_KEYDOWN 128
#define GRID_PARAMETER_KEYBOARD_KEYUP 129
#define GRID_PARAMETER_KEYBOARD_MODIFIER 130
#define GRID_PARAMETER_KEYBOARD_NOTMODIFIER 131

// HEADER BROADCAST

#define GRID_BRC_frame "%c%c............%c", GRID_CONST_SOH, GRID_CONST_BRC, GRID_CONST_EOB

#define GRID_BRC_LEN_offset 2
#define GRID_BRC_LEN_length 2

#define GRID_BRC_ID_offset 4
#define GRID_BRC_ID_length 2

#define GRID_BRC_DX_offset 6
#define GRID_BRC_DX_length 2

#define GRID_BRC_DY_offset 8
#define GRID_BRC_DY_length 2

#define GRID_BRC_AGE_offset 10
#define GRID_BRC_AGE_length 2

#define GRID_BRC_ROT_offset 12
#define GRID_BRC_ROT_length 2



#define GRID_INSTR_length 1
#define GRID_INSTR_offset 4

// Save the following action to the given event & change instruction to execute
#define GRID_INSTR_ACKNOWLEDGE_code 0xA //a

#define GRID_INSTR_NACKNOWLEDGE_code 0xB //b

#define GRID_INSTR_CONFIGURE_code 0xC //c

#define GRID_INSTR_REPORT_code 0xD //c

// Respond with executable please!
#define GRID_INSTR_FETCH_code 0xF //e

// Execute The Action if possible!
#define GRID_INSTR_EXECUTE_code 0xE //f

#define GRID_CLASS_length 3
#define GRID_CLASS_offset 1



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


// HEARTBEAT (type=0 grid, type=1 gridmaster, type=255 editor)
#define GRID_CLASS_HEARTBEAT_code 0x010
#define GRID_CLASS_HEARTBEAT_frame "%c%03x_..........%c", GRID_CONST_STX, GRID_CLASS_HEARTBEAT_code, GRID_CONST_ETX

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
#define GRID_CLASS_UPTIME_UPTIME_length 8

// DEBUGTEXT
#define GRID_CLASS_DEBUGTEXT_code 0x020
#define GRID_CLASS_DEBUGTEXT_frame_start "%c%03xe", GRID_CONST_STX, GRID_CLASS_DEBUGTEXT_code
#define GRID_CLASS_DEBUGTEXT_frame_end "%c", GRID_CONST_ETX

// BANK
#define GRID_CLASS_BANKACTIVE_code 0x030
#define GRID_CLASS_BANKACTIVE_format "%c%03x%01x%02x%c", GRID_CONST_STX, GRID_CLASS_BANKACTIVE_code, instruction, banknumber, GRID_CONST_ETX
#define GRID_CLASS_BANKACTIVE_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_BANKACTIVE_code, GRID_CONST_ETX

#define GRID_CLASS_BANKACTIVE_BANKNUMBER_offset 5
#define GRID_CLASS_BANKACTIVE_BANKNUMBER_length 2

#define GRID_CLASS_BANKENABLED_code 0x031
#define GRID_CLASS_BANKENABLED_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_BANKENABLED_code, GRID_CONST_ETX

#define GRID_CLASS_BANKENABLED_BANKNUMBER_offset 5
#define GRID_CLASS_BANKENABLED_BANKNUMBER_length 2

#define GRID_CLASS_BANKENABLED_ISENABLED_offset 7
#define GRID_CLASS_BANKENABLED_ISENABLED_length 2

#define GRID_CLASS_BANKCOLOR_code 0x032
#define GRID_CLASS_BANKCOLOR_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_BANKCOLOR_code, GRID_CONST_ETX

#define GRID_CLASS_BANKCOLOR_NUM_offset 5
#define GRID_CLASS_BANKCOLOR_NUM_length 2

#define GRID_CLASS_BANKCOLOR_RED_offset 7
#define GRID_CLASS_BANKCOLOR_RED_length 2

#define GRID_CLASS_BANKCOLOR_GRE_offset 9
#define GRID_CLASS_BANKCOLOR_GRE_length 2

#define GRID_CLASS_BANKCOLOR_BLU_offset 11
#define GRID_CLASS_BANKCOLOR_BLU_length 2

// LED SET PHASE

#define GRID_CLASS_LEDPHASE_code 0x040
#define GRID_CLASS_LEDPHASE_format "%c%03x%01x%02x%02x%02x%c", GRID_CONST_STX, GRID_CLASS_LEDPHASE_code, instruction, layernumber, lednumber, phase, GRID_CONST_ETX
#define GRID_CLASS_LEDPHASE_frame "%c%03x_......%c", GRID_CONST_STX, GRID_CLASS_LEDPHASE_code, GRID_CONST_ETX

#define GRID_CLASS_LEDPHASE_NUM_offset 5
#define GRID_CLASS_LEDPHASE_NUM_length 2

#define GRID_CLASS_LEDPHASE_LAY_offset 7
#define GRID_CLASS_LEDPHASE_LAY_length 2

#define GRID_CLASS_LEDPHASE_PHA_offset 9
#define GRID_CLASS_LEDPHASE_PHA_length 2

// LED SET COLOR

#define GRID_CLASS_LEDCOLOR_code 0x041
#define GRID_CLASS_LEDCOLOR_format "%c%03x%01x%02x%02x%02x%02x%02x%c", GRID_CONST_STX, GRID_CLASS_LEDPHASE_code, instruction, layernumber, lednumber, red, gre, blu, GRID_CONST_ETX
#define GRID_CLASS_LEDCOLOR_frame "%c%03x_..........%c", GRID_CONST_STX, GRID_CLASS_LEDCOLOR_code, GRID_CONST_ETX

#define GRID_CLASS_LEDCOLOR_NUM_offset 5
#define GRID_CLASS_LEDCOLOR_NUM_length 2

#define GRID_CLASS_LEDCOLOR_LAY_offset 7
#define GRID_CLASS_LEDCOLOR_LAY_length 2

#define GRID_CLASS_LEDCOLOR_RED_offset 9
#define GRID_CLASS_LEDCOLOR_RED_length 2

#define GRID_CLASS_LEDCOLOR_GRE_offset 11
#define GRID_CLASS_LEDCOLOR_GRE_length 2

#define GRID_CLASS_LEDCOLOR_BLU_offset 13
#define GRID_CLASS_LEDCOLOR_BLU_length 2

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

#define GRID_TEMPLATE_UI_PARAMETER_LIST_LENGTH 20



#define GRID_LUA_FNC_G_LED_PHASE_short "glp"
#define GRID_LUA_FNC_G_LED_PHASE_human "led_value"
#define GRID_LUA_FNC_G_LED_PHASE_fnptr l_grid_led_set_phase

#define GRID_LUA_FNC_G_LED_MIN_short "gln"
#define GRID_LUA_FNC_G_LED_MIN_human "led_color_min"
#define GRID_LUA_FNC_G_LED_MIN_fnptr l_grid_led_set_min

#define GRID_LUA_FNC_G_LED_MID_short "gld"
#define GRID_LUA_FNC_G_LED_MID_human "led_color_mid"
#define GRID_LUA_FNC_G_LED_MID_fnptr l_grid_led_set_mid

#define GRID_LUA_FNC_G_LED_MAX_short "glx"
#define GRID_LUA_FNC_G_LED_MAX_human "led_color_max"
#define GRID_LUA_FNC_G_LED_MAX_fnptr l_grid_led_set_max

#define GRID_LUA_FNC_G_LED_COLOR_short "glc"
#define GRID_LUA_FNC_G_LED_COLOR_human "led_color"
#define GRID_LUA_FNC_G_LED_COLOR_fnptr l_grid_led_set_color

#define GRID_LUA_FNC_G_LED_FREQUENCY_short "glf"
#define GRID_LUA_FNC_G_LED_FREQUENCY_human "led_animation_rate"
#define GRID_LUA_FNC_G_LED_FREQUENCY_fnptr l_grid_led_set_frequency

#define GRID_LUA_FNC_G_LED_SHAPE_short "gls"
#define GRID_LUA_FNC_G_LED_SHAPE_human "led_animation_type"
#define GRID_LUA_FNC_G_LED_SHAPE_fnptr l_grid_led_set_phase

#define GRID_LUA_FNC_G_LED_PSF_short "glpfs"
#define GRID_LUA_FNC_G_LED_PSF_human "led_animation_phase_rate_type"
#define GRID_LUA_FNC_G_LED_PSF_fnptr l_grid_led_set_pfs

#define GRID_LUA_FNC_G_MIDI_SEND_short "gms"
#define GRID_LUA_FNC_G_MIDI_SEND_human "midi_send"
#define GRID_LUA_FNC_G_MIDI_SEND_fnptr l_grid_midi_send

/*

uptime
resetcause
memory states
version
stored config version
stored config date

*/


// ========================= ENCODER =========================== //

#define GRID_LUA_FNC_E_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_E_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_E_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_E_BUTTON_NUMBER_helper "1"
#define GRID_LUA_FNC_E_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_E_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_E_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_E_BUTTON_VALUE_helper "2"
#define GRID_LUA_FNC_E_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_E_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_E_BUTTON_MIN_index 3
#define GRID_LUA_FNC_E_BUTTON_MIN_helper "3"
#define GRID_LUA_FNC_E_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_E_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_E_BUTTON_MAX_index 4
#define GRID_LUA_FNC_E_BUTTON_MAX_helper "4"
#define GRID_LUA_FNC_E_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_E_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_E_BUTTON_MODE_index 5
#define GRID_LUA_FNC_E_BUTTON_MODE_helper "5"
#define GRID_LUA_FNC_E_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_E_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_E_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_helper "6"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_E_BUTTON_STATE_index 7
#define GRID_LUA_FNC_E_BUTTON_STATE_helper "7"
#define GRID_LUA_FNC_E_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_E_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_E_ENCODER_NUMBER_index 8
#define GRID_LUA_FNC_E_ENCODER_NUMBER_helper "8"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_short "enu"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_human "encoder_number"

#define GRID_LUA_FNC_E_ENCODER_VALUE_index 9
#define GRID_LUA_FNC_E_ENCODER_VALUE_helper "9"
#define GRID_LUA_FNC_E_ENCODER_VALUE_short "eva"
#define GRID_LUA_FNC_E_ENCODER_VALUE_human "encoder_value"

#define GRID_LUA_FNC_E_ENCODER_MIN_index 10
#define GRID_LUA_FNC_E_ENCODER_MIN_helper "10"
#define GRID_LUA_FNC_E_ENCODER_MIN_short "emi"
#define GRID_LUA_FNC_E_ENCODER_MIN_human "encoder_min"

#define GRID_LUA_FNC_E_ENCODER_MAX_index 11
#define GRID_LUA_FNC_E_ENCODER_MAX_helper "11"
#define GRID_LUA_FNC_E_ENCODER_MAX_short "ema"
#define GRID_LUA_FNC_E_ENCODER_MAX_human "encoder_max"

#define GRID_LUA_FNC_E_ENCODER_MODE_index 12
#define GRID_LUA_FNC_E_ENCODER_MODE_helper "12"
#define GRID_LUA_FNC_E_ENCODER_MODE_short "emo"
#define GRID_LUA_FNC_E_ENCODER_MODE_human "encoder_mode"

#define GRID_LUA_FNC_E_ENCODER_ELAPSED_index 13
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_helper "13"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_short "eel"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_human "encoder_elapsed_time"

// Encoder parameters
#define GRID_LUA_FNC_E_LIST_length 14

// Encoder init function
#define GRID_LUA_E_LIST_init "init_encoder = function (e, i) \
	 \
	e." GRID_LUA_FNC_E_ELEMENT_INDEX_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ELEMENT_INDEX_helper ", a) end \
	\
	e." GRID_LUA_FNC_E_BUTTON_NUMBER_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_NUMBER_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_VALUE_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_VALUE_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_MIN_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_MIN_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_MAX_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_MAX_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_MODE_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_MODE_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_ELAPSED_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_ELAPSED_helper ", a) end \
	e." GRID_LUA_FNC_E_BUTTON_STATE_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_BUTTON_STATE_helper ", a) end \
	\
	e." GRID_LUA_FNC_E_ENCODER_NUMBER_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_NUMBER_helper ", a) end \
	e." GRID_LUA_FNC_E_ENCODER_VALUE_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_VALUE_helper ", a) end \
	e." GRID_LUA_FNC_E_ENCODER_MIN_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_MIN_helper ", a) end \
	e." GRID_LUA_FNC_E_ENCODER_MAX_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_MAX_helper ", a) end \
	e." GRID_LUA_FNC_E_ENCODER_MODE_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_MODE_helper ", a) end \
	e." GRID_LUA_FNC_E_ENCODER_ELAPSED_short "=function (a) return gtv(i, " GRID_LUA_FNC_E_ENCODER_ELAPSED_helper ", a) end \
    end"

// ========================= POTMETER =========================== //

#define GRID_LUA_FNC_P_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_P_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_P_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_P_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_P_POTMETER_NUMBER_index 1
#define GRID_LUA_FNC_P_POTMETER_NUMBER_helper "1"
#define GRID_LUA_FNC_P_POTMETER_NUMBER_short "pnu"
#define GRID_LUA_FNC_P_POTMETER_NUMBER_human "potmeter_number"

#define GRID_LUA_FNC_P_POTMETER_VALUE_index 2
#define GRID_LUA_FNC_P_POTMETER_VALUE_helper "2"
#define GRID_LUA_FNC_P_POTMETER_VALUE_short "pva"
#define GRID_LUA_FNC_P_POTMETER_VALUE_human "potmeter_value"

#define GRID_LUA_FNC_P_POTMETER_MIN_index 3
#define GRID_LUA_FNC_P_POTMETER_MIN_helper "3"
#define GRID_LUA_FNC_P_POTMETER_MIN_short "pmi"
#define GRID_LUA_FNC_P_POTMETER_MIN_human "potmeter_min"

#define GRID_LUA_FNC_P_POTMETER_MAX_index 4
#define GRID_LUA_FNC_P_POTMETER_MAX_helper "4"
#define GRID_LUA_FNC_P_POTMETER_MAX_short "pma"
#define GRID_LUA_FNC_P_POTMETER_MAX_human "potmeter_max"

#define GRID_LUA_FNC_P_POTMETER_MODE_index 5
#define GRID_LUA_FNC_P_POTMETER_MODE_helper "5"
#define GRID_LUA_FNC_P_POTMETER_MODE_short "pmo"
#define GRID_LUA_FNC_P_POTMETER_MODE_human "potmeter_resolution"

#define GRID_LUA_FNC_P_POTMETER_ELAPSED_index 6
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_helper "6"
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_short "pel"
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_human "potmeter_elapsed_time"

// Encoder parameters
#define GRID_LUA_FNC_P_LIST_length 7

// Encoder init function
#define GRID_LUA_P_LIST_init "init_potmeter = function (e, i) \
	 \
	e." GRID_LUA_FNC_P_ELEMENT_INDEX_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_ELEMENT_INDEX_helper ", a) end \
	\
	e." GRID_LUA_FNC_P_POTMETER_NUMBER_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_NUMBER_helper ", a) end \
	e." GRID_LUA_FNC_P_POTMETER_VALUE_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_VALUE_helper ", a) end \
	e." GRID_LUA_FNC_P_POTMETER_MIN_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_MIN_helper ", a) end \
	e." GRID_LUA_FNC_P_POTMETER_MAX_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_MAX_helper ", a) end \
	e." GRID_LUA_FNC_P_POTMETER_MODE_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_MODE_helper ", a) end \
	e." GRID_LUA_FNC_P_POTMETER_ELAPSED_short "=function (a) return gtv(i, " GRID_LUA_FNC_P_POTMETER_ELAPSED_helper ", a) end \
	\
    end"


// ========================= BUTTON =========================== //

#define GRID_LUA_FNC_B_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_B_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_B_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_B_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_B_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_B_BUTTON_NUMBER_helper "1"
#define GRID_LUA_FNC_B_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_B_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_B_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_B_BUTTON_VALUE_helper "2"
#define GRID_LUA_FNC_B_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_B_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_B_BUTTON_MIN_index 3
#define GRID_LUA_FNC_B_BUTTON_MIN_helper "3"
#define GRID_LUA_FNC_B_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_B_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_B_BUTTON_MAX_index 4
#define GRID_LUA_FNC_B_BUTTON_MAX_helper "4"
#define GRID_LUA_FNC_B_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_B_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_B_BUTTON_MODE_index 5
#define GRID_LUA_FNC_B_BUTTON_MODE_helper "5"
#define GRID_LUA_FNC_B_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_B_BUTTON_MODE_human "button_resolution"

#define GRID_LUA_FNC_B_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_helper "6"
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_B_BUTTON_STATE_index 7
#define GRID_LUA_FNC_B_BUTTON_STATE_helper "7"
#define GRID_LUA_FNC_B_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_B_BUTTON_STATE_human "button_state"

// Encoder parameters
#define GRID_LUA_FNC_B_LIST_length 8

// Encoder init function
#define GRID_LUA_B_LIST_init "init_button = function (e, i) \
	 \
	e." GRID_LUA_FNC_B_ELEMENT_INDEX_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_ELEMENT_INDEX_helper ", a) end \
	\
	e." GRID_LUA_FNC_B_BUTTON_NUMBER_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_NUMBER_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_VALUE_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_VALUE_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_MIN_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_MIN_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_MAX_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_MAX_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_MODE_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_MODE_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_ELAPSED_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_ELAPSED_helper ", a) end \
	e." GRID_LUA_FNC_B_BUTTON_STATE_short "=function (a) return gtv(i, " GRID_LUA_FNC_B_BUTTON_STATE_helper ", a) end \
	\
    end"


#define GRID_LUA_KW_ELEMENT_short "ele"
#define GRID_LUA_KW_THIS_short "this"

// element[4].encoder_value()
// ele[4].eva()

// Global parameters
#define GRID_TEMPLATE_Z_PARAMETER_LIST_LENGTH 6

enum grid_template_z_parameter_index_t
{

	GRID_TEMPLATE_Z_PARAMETER_BANK_NUMBER_ACTIVE,

	GRID_TEMPLATE_Z_PARAMETER_BANK_COLOR_RED,
	GRID_TEMPLATE_Z_PARAMETER_BANK_COLOR_GRE,
	GRID_TEMPLATE_Z_PARAMETER_BANK_COLOR_BLU,

	GRID_TEMPLATE_Z_PARAMETER_MAPMODE_STATE,
	GRID_TEMPLATE_Z_PARAMETER_BANK_NEXT,

};

enum grid_ui_element_t
{

	GRID_UI_ELEMENT_SYSTEM,
	GRID_UI_ELEMENT_POTENTIOMETER,
	GRID_UI_ELEMENT_BUTTON,
	GRID_UI_ELEMENT_ENCODER,

};

#define GRID_PARAMETER_ELEMENTTYPE_SYSTEM_code "00"
#define GRID_PARAMETER_ELEMENTTYPE_POTENTIOMETER_code "01"
#define GRID_PARAMETER_ELEMENTTYPE_BUTTON_code "02"
#define GRID_PARAMETER_ELEMENTTYPE_ENCODER_code "03"

enum grid_ui_event_t
{

	GRID_UI_EVENT_INIT,

	GRID_UI_EVENT_AC,
	GRID_UI_EVENT_EC,
	GRID_UI_EVENT_BC,
	GRID_UI_EVENT_MAPMODE_PRESS,
	GRID_UI_EVENT_MAPMODE_RELEASE,

	GRID_UI_EVENT_CFG_RESPONSE,
	GRID_UI_EVENT_CFG_REQUEST,
	GRID_UI_EVENT_CFG_EDITOR,

	GRID_UI_EVENT_HEARTBEAT,

};

// BANK + ELEMENT NUMBER + EVENT TYPE + PARAMETER

#define GRID_EVENTSTRING_HEARTBEAT "\x02" \
								   "050e<?expr p(Z0) ?>000c00\x03"

#define GRID_EVENTSTRING_AC "\x02" \
								"050e<?expr p(Z0) ?><?expr p(T0) ?>01<?expr p(T2) ?>\x03"

#define GRID_EVENTSTRING_BC "\x02" \
							"050e<?expr p(Z0) ?><?expr p(T0) ?>04<?expr p(T5) ?>\x03"

#define GRID_EVENTSTRING_EC "\x02" \
							"050e<?expr p(Z0) ?><?expr p(T0) ?>01<?expr p(T5) ?>\x03"

#define GRID_EVENTSTRING_DP_ENC "\x02" \
								"050e<?expr p(Z0) ?><?expr p(T0) ?>04<?expr p(T2) ?>\x03"
#define GRID_EVENTSTRING_DR_ENC "\x02" \
								"050e<?expr p(Z0) ?><?expr p(T0) ?>05<?expr p(T2) ?>\x03"

#define GRID_EVENTSTRING_INIT_POT "\x02" \
								  "050e<?expr p(Z0) ?><?expr p(T0) ?>0000\x03"
#define GRID_EVENTSTRING_INIT_BUT "\x02" \
								  "050e<?expr p(Z0) ?><?expr p(T0) ?>0000\x03"
#define GRID_EVENTSTRING_INIT_ENC "\x02" \
								  "050e<?expr p(Z0) ?><?expr p(T0) ?>0000\x03"

#define GRID_EVENTSTRING_MAPMODE_PRESS "\x02" \
									   "050e<?expr p(Z0) ?>0008<?expr p(Z4) ?>\x03"
#define GRID_EVENTSTRING_MAPMODE_RELEASE "\x02" \
										 "050e<?expr p(Z0) ?>0009<?expr p(Z4) ?>\x03"

#define GRID_EVENTSTRING_CFG_RESPONES "\x02" \
									  "050e<?expr p(Z0) ?>000a<?expr p(Z4) ?>\x03"
#define GRID_EVENTSTRING_CFG_REQUEST "\x02" \
									 "050e<?expr p(Z0) ?>000b<?expr p(Z4) ?>\x03"

// DEFAULT ACTION:                     FIRST MIDI then LED_SET_PHASE

#define GRID_ACTIONSTRING_INIT_POT "\x02" \
								   "041e<?expr p(T0) ?>01<?expr p(Z1) ?><?expr p(Z2) ?><?expr p(Z3) ?>\x03"
#define GRID_ACTIONSTRING_INIT_BUT "\x02" \
								   "041e<?expr p(T0) ?>01<?expr p(Z1) ?><?expr p(Z2) ?><?expr p(Z3) ?>\x03"
#define GRID_ACTIONSTRING_INIT_ENC "<\x02"                                                                      \
								   "041e<?expr p(T0) ?>01<?expr p(Z1) ?><?expr p(Z2) ?><?expr p(Z3) ?>\x03\x02" \
								   "041e<?expr p(T0) ?>02<?expr p(Z1) ?><?expr p(Z2) ?><?expr p(Z3) ?>\x03"

#define GRID_ACTIONSTRING_AC			"<?lua gms(0,176,this.ind(),this.pva()) glp(this.ind(), 1) ?>"

//#define GRID_ACTIONSTRING_AC_POT "<?lua this.T[4]=16383 this.T[5]=9 gsm(0,176,this.T[0],this.T[2]//128) gsm(0,176,this.T[0]+32,this.T[2]%%128) glsp(this.T[0], 1) ?>"

// 14bit     gsm(0,176,this.T[0],this.T[2]//128) gsm(0,176,this.T[0]+32,this.T[2]%%128)
//#define GRID_ACTIONSTRING_AC_POT			"<?lua this.T[4]=16383 this.T[5]=9  gslp(this.T[0], 1) ?>"

#define GRID_ACTIONSTRING_BC "<?lua gln(this.ind(),2,255,7,0) if (this.bva()>0) then glpfs(this.ind(),2,0,1,(this.ind())%%4) else glpfs(this.ind(), 2, 0, 0, 0) end ?>"

//#define GRID_ACTIONSTRING_EC				"<?lua grid_send_midi(0,176,this.enu(),this.eva()) grid_led_set_phase(this.ind(), 1, this.eva()) ?>"
// 14bit midi test
// #define GRID_ACTIONSTRING_EC				"<?lua this.T[11]=16383 gsm(0,176,this.T[0],this.T[9]//128) gsm(0,176,this.T[0]+32,this.T[9]%%128) glsp(this.T[0], 1, this.T[9]//128) ?>"

#define GRID_ACTIONSTRING_EC "<?lua gms(0, 176, this.ind(), this.eva()) gld(this.ind(), 1 , 0, 130, 0) glp(this.ind(), 1, this.eva()) ?>"

#define GRID_ACTIONSTRING_MAPMODE_PRESS "\x02" \
										"030e<?expr p(Z5) ?>\x03"
#define GRID_ACTIONSTRING_MAPMODE_RELEASE ""

#define GRID_ACTIONSTRING_CFG_RESPONSE "\x02" \
									   "030e<?expr p(Z0) ?>\x03"
#define GRID_ACTIONSTRING_CFG_REQUEST "\x02" \
									  "030f<?expr p(Z0) ?>\x03"

#define GRID_CLASS_EVENT_code 0x050
#define GRID_CLASS_EVENT_frame "%c%03x_........%c", GRID_CONST_STX, GRID_CLASS_EVENT_code, GRID_CONST_ETX

#define GRID_CLASS_EVENT_BANKNUMBER_offset 5
#define GRID_CLASS_EVENT_BANKNUMBER_length 2

#define GRID_CLASS_EVENT_ELEMENTNUMBER_offset 7
#define GRID_CLASS_EVENT_ELEMENTNUMBER_length 2

#define GRID_CLASS_EVENT_EVENTTYPE_offset 9
#define GRID_CLASS_EVENT_EVENTTYPE_length 2

#define GRID_CLASS_EVENT_EVENTPARAM_offset 11
#define GRID_CLASS_EVENT_EVENTPARAM_length 2

#define GRID_CLASS_GLOBALSTORE_code 0x060
#define GRID_CLASS_GLOBALSTORE_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_GLOBALSTORE_code, GRID_CONST_ETX

#define GRID_CLASS_GLOBALLOAD_code 0x061
#define GRID_CLASS_GLOBALLOAD_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_GLOBALLOAD_code, GRID_CONST_ETX

#define GRID_CLASS_GLOBALCLEAR_code 0x062
#define GRID_CLASS_GLOBALCLEAR_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_GLOBALCLEAR_code, GRID_CONST_ETX

#define GRID_CLASS_GLOBALRECALL_code 0x063
#define GRID_CLASS_GLOBALRECALL_frame "%c%03xe..%c", GRID_CONST_STX, GRID_CLASS_GLOBALRECALL_code, GRID_CONST_ETX

#define GRID_CLASS_GLOBALRECALL_BANKNUMBER_offset 5
#define GRID_CLASS_GLOBALRECALL_BANKNUMBER_length 2

#define GRID_CLASS_LOCALSTORE_code 0x070
#define GRID_CLASS_LOCALSTORE_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_LOCALSTORE_code, GRID_CONST_ETX

#define GRID_CLASS_LOCALLOAD_code 0x071
#define GRID_CLASS_LOCALLOAD_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_LOCALLOAD_code, GRID_CONST_ETX

#define GRID_CLASS_LOCALCLEAR_code 0x072
#define GRID_CLASS_LOCALCLEAR_frame "%c%03xe%c", GRID_CONST_STX, GRID_CLASS_LOCALCLEAR_code, GRID_CONST_ETX

// CONFIG STORE     Fetch(Read) Configure(Overwrite) Append(Write)  ////// DEPRICATED
#define GRID_CLASS_CONFIGURATION_code 0x080
#define GRID_CLASS_CONFIGURATION_frame "%c%03x_......%c", GRID_CONST_STX, GRID_CLASS_CONFIGURATION_code, GRID_CONST_ETX
#define GRID_CLASS_CONFIGURATION_frame_start "%c%03x_......", GRID_CONST_STX, GRID_CLASS_CONFIGURATION_code
#define GRID_CLASS_CONFIGURATION_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_CONFIGURATION_BANKNUMBER_offset 5
#define GRID_CLASS_CONFIGURATION_BANKNUMBER_length 2

#define GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset 7
#define GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length 2

#define GRID_CLASS_CONFIGURATION_EVENTTYPE_offset 9
#define GRID_CLASS_CONFIGURATION_EVENTTYPE_length 2

#define GRID_CLASS_CONFIGURATION_ACTIONSTRING_offset 11
#define GRID_CLASS_CONFIGURATION_ACTIONSTRING_length 0

// CONFIG STORE     Fetch(Read) Configure(Overwrite) Append(Write)
#define GRID_CLASS_CONFIGDEFAULT_code 0x081
#define GRID_CLASS_CONFIGDEFAULT_frame "%c%03x_......%c", GRID_CONST_STX, GRID_CLASS_CONFIGDEFAULT_code, GRID_CONST_ETX
#define GRID_CLASS_CONFIGDEFAULT_frame_start "%c%03x_......", GRID_CONST_STX, GRID_CLASS_CONFIGDEFAULT_code
#define GRID_CLASS_CONFIGDEFAULT_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_CONFIGDEFAULT_BANKNUMBER_offset 5
#define GRID_CLASS_CONFIGDEFAULT_BANKNUMBER_length 2

#define GRID_CLASS_CONFIGDEFAULT_ELEMENTNUMBER_offset 7
#define GRID_CLASS_CONFIGDEFAULT_ELEMENTNUMBER_length 2

#define GRID_CLASS_CONFIGDEFAULT_EVENTTYPE_offset 9
#define GRID_CLASS_CONFIGDEFAULT_EVENTTYPE_length 2

// CONFIG STORE     Fetch(Read) Configure(Overwrite) Append(Write)
#define GRID_CLASS_CONFIG_code 0x082
#define GRID_CLASS_CONFIG_frame "%c%03x_..........%c", GRID_CONST_STX, GRID_CLASS_CONFIG_code, GRID_CONST_ETX
#define GRID_CLASS_CONFIG_frame_start "%c%03x_..........", GRID_CONST_STX, GRID_CLASS_CONFIG_code
#define GRID_CLASS_CONFIG_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_CONFIG_PAGENUMBER_offset 5
#define GRID_CLASS_CONFIG_PAGENUMBER_length 2

#define GRID_CLASS_CONFIG_ELEMENTNUMBER_offset 7
#define GRID_CLASS_CONFIG_ELEMENTNUMBER_length 2

#define GRID_CLASS_CONFIG_EVENTTYPE_offset 9
#define GRID_CLASS_CONFIG_EVENTTYPE_length 2

#define GRID_CLASS_CONFIG_ACTIONLENGTH_offset 11
#define GRID_CLASS_CONFIG_ACTIONLENGTH_length 4

#define GRID_CLASS_CONFIG_ACTIONSTRING_offset 15
#define GRID_CLASS_CONFIG_ACTIONSTRING_length 0

// 090 HID KEYBOARD STATUS

#define GRID_CLASS_HIDKEYSTATUS_code 0x090
#define GRID_CLASS_HIDKEYSTATUS_frame "%c%03x_..%c", GRID_CONST_STX, GRID_CLASS_HIDKEYSTATUS_code, GRID_CONST_ETX

#define GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset 5
#define GRID_CLASS_HIDKEYSTATUS_ISENABLED_length 2

// 091 HID KEYBOARD LOWLEVEL KEYPRESS/RELEASE

#define GRID_CLASS_HIDKEYBOARD_code 0x091
#define GRID_CLASS_HIDKEYBOARD_frame "%c%03x_....%c", GRID_CONST_STX, GRID_CLASS_HIDKEYBOARD_code, GRID_CONST_ETX
#define GRID_CLASS_HIDKEYBOARD_frame_start "%c%03x_", GRID_CONST_STX, GRID_CLASS_HIDKEYBOARD_code
#define GRID_CLASS_HIDKEYBOARD_frame_end "%c", GRID_CONST_ETX

#define GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset 5
#define GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length 1

#define GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset 6
#define GRID_CLASS_HIDKEYBOARD_KEYSTATE_length 1

#define GRID_CLASS_HIDKEYBOARD_KEYCODE_offset 7
#define GRID_CLASS_HIDKEYBOARD_KEYCODE_length 2

#endif /* GRID_PROTOCOL_H_INCLUDED */
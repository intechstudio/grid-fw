#ifndef GRID_PROTOCOL_H_INCLUDED
#define GRID_PROTOCOL_H_INCLUDED

#define GRID_PROTOCOL_VERSION_MAJOR		1
#define GRID_PROTOCOL_VERSION_MINOR		0
#define GRID_PROTOCOL_VERSION_PATCH		2


// Module HWCFG definitions

#define  GRID_MODULE_PO16_RevB		0
#define  GRID_MODULE_PO16_RevC		8

#define  GRID_MODULE_BU16_RevB		128
#define  GRID_MODULE_BU16_RevC		136

#define  GRID_MODULE_PBF4_RevA		64
#define  GRID_MODULE_EN16_RevA		192



#define GRID_CONST_NUL				0x00

#define GRID_CONST_SOH				0x01	//start of header
#define GRID_CONST_STX				0x02	//start of text
#define GRID_CONST_ETX				0x03	//end of text
#define GRID_CONST_EOB				0x17	//end of block
#define GRID_CONST_EOT				0x04	//end of transmission

#define GRID_CONST_LF				0x0A	//linefeed, newline

#define GRID_CONST_ACK				0x06	//acknowledge
#define GRID_CONST_NAK				0x15	//nacknowledge	
#define GRID_CONST_CAN				0x18	//cancel

#define GRID_CONST_NORTH			0x11	// Device Control 1
#define GRID_CONST_EAST				0x12	// Device Control 2
#define GRID_CONST_SOUTH			0x13	// Device Control 3
#define GRID_CONST_WEST				0x14	// Device Control 4

#define GRID_CONST_DIRECT			0x0E	// Shift In
#define GRID_CONST_BROADCAST		0x0F	// Shift Out

#define GRID_CONST_BELL				0x07	


#define GRID_CLASS_MIDI										0
	#define GRID_COMMAND_MIDI_NOTEOFF						0x80
	#define GRID_COMMAND_MIDI_NOTEON						0x90
	#define GRID_COMMAND_MIDI_CONTROLCHANGE					176
	#define GRID_COMMAND_MIDI_ENCODERCHANGE					200

#define GRID_CLASS_KEYBOARD									1
	#define GRID_COMMAND_KEYBOARD_KEYDOWN					128
	#define GRID_COMMAND_KEYBOARD_KEYUP						129
	#define GRID_PARAMETER_KEYBOARD_MODIFIER				130
	#define GRID_PARAMETER_KEYBOARD_NOTMODIFIER				131
	
#define GRID_CLASS_MOUSE									2

#define GRID_CLASS_LED										3
	#define GRID_COMMAND_LED_SETPHASE						99
	#define GRID_COMMAND_LED_SETCOLOR						100
	
#define GRID_CLASS_SYS										4
	#define GRID_COMMAND_SYS_BANK							100
	#define GRID_PARAMETER_SYS_BANKSELECT					101

	#define GRID_COMMAND_SYS_HEARTBEAT						102
	#define GRID_PARAMETER_SYS_HEARTBEATALIVE				103

	#define GRID_COMMAND_SYS_CFG							104
	#define GRID_PARAMETER_SYS_CFGREQUEST					105





#endif /* GRID_PROTOCOL_H_INCLUDED */
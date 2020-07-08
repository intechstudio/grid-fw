#ifndef GRID_PROTOCOL_H_INCLUDED
#define GRID_PROTOCOL_H_INCLUDED

#define GRID_PROTOCOL_VERSION_MAJOR		1
#define GRID_PROTOCOL_VERSION_MINOR		0
#define GRID_PROTOCOL_VERSION_PATCH		5


// Module HWCFG definitions

#define  GRID_MODULE_PO16_RevB		0
#define  GRID_MODULE_PO16_RevC		8

#define  GRID_MODULE_BU16_RevB		128
#define  GRID_MODULE_BU16_RevC		136

#define  GRID_MODULE_PBF4_RevA		64
#define  GRID_MODULE_EN16_RevA		192

#define  GRID_PARAMETER_HEARTBEAT_INTERVAL	250
#define  GRID_PARAMETER_PING_INTERVAL		100



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

#define GRID_CONST_DCT				0x0E	// Shift In = Direct
#define GRID_CONST_BRC				0x0F	// Shift Out = Broadcast

#define GRID_CONST_BELL				0x07 	





#define GRID_PARAMETER_MIDI_NOTEOFF							0x80
#define GRID_PARAMETER_MIDI_NOTEON							0x90
#define GRID_PARAMETER_MIDI_CONTROLCHANGE					0xB0


#define GRID_PARAMETER_KEYBOARD_KEYDOWN						128
#define GRID_PARAMETER_KEYBOARD_KEYUP						129
#define GRID_PARAMETER_KEYBOARD_MODIFIER					130
#define GRID_PARAMETER_KEYBOARD_NOTMODIFIER					131
 	



// HEADER BROADCAST

#define GRID_BRC_format				"%c%c%02x%02x%02x%02x%02x%02x%c",GRID_CONST_SOH,GRID_CONST_BRC,len,id,dx,dy,age,rot,GRID_CONST_EOB
#define GRID_BRC_frame				"%c%c............%c",GRID_CONST_SOH,GRID_CONST_BRC,GRID_CONST_EOB


#define GRID_BRC_LEN_offset			2
#define GRID_BRC_LEN_length			2

#define GRID_BRC_ID_offset			4
#define GRID_BRC_ID_length			2

#define GRID_BRC_DX_offset			6
#define GRID_BRC_DX_length			2

#define GRID_BRC_DY_offset			8
#define GRID_BRC_DY_length			2

#define GRID_BRC_AGE_offset			10
#define GRID_BRC_AGE_length			2

#define GRID_BRC_ROT_offset			12
#define GRID_BRC_ROT_length			2


//NEW ERA


#define GRID_INSTR_length					1
#define GRID_INSTR_offset					4

#define GRID_INSTR_REQ_code					0xE
#define GRID_INSTR_REP_code					0xF


#define GRID_CLASS_length					3
#define GRID_CLASS_offset					1

// MIDI
#define GRID_CLASS_MIDIRELATIVE_code		0x000
#define GRID_CLASS_MIDIRELATIVE_format		"%c%03x%01x%02x%02x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_MIDIRELATIVE_code,instruction,cablechannel,channelcommand,param1,param2,GRID_CONST_ETX
#define GRID_CLASS_MIDIRELATIVE_frame		"%c%03x_........%c",GRID_CONST_STX,GRID_CLASS_MIDIRELATIVE_code,GRID_CONST_ETX


#define GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_offset			5
#define GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_length			2

#define GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_offset		7
#define GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_length		2

#define GRID_CLASS_MIDIRELATIVE_PARAM1_offset				9
#define GRID_CLASS_MIDIRELATIVE_PARAM1_length				2

#define GRID_CLASS_MIDIRELATIVE_PARAM2_offset				11
#define GRID_CLASS_MIDIRELATIVE_PARAM2_length				2

//#define GRID_CLASS_MIDIABSOLUTE_code		0x001
//#define GRID_CLASS_MIDIABSOLUTE_format		"%c%03x%01x%02x%02x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_MIDIABSOLUTE_code,instruction,cablechannel,channelcommand,param1,param2,GRID_CONST_ETX

// HEARTBEAT
#define GRID_CLASS_HEARTBEAT_code			0x010
#define GRID_CLASS_HEARTBEAT_format			"%c%03x%01x%02x%02x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_HEARTBEAT_code,instruction,hwcfg,vmajor,vminor,vpatch,GRID_CONST_ETX
#define GRID_CLASS_HEARTBEAT_frame			"%c%03x_........%c",GRID_CONST_STX,GRID_CLASS_HEARTBEAT_code,GRID_CONST_ETX

#define GRID_CLASS_HEARTBEAT_HWCFG_offset		5
#define GRID_CLASS_HEARTBEAT_HWCFG_length		2

#define GRID_CLASS_HEARTBEAT_VMAJOR_offset		7
#define GRID_CLASS_HEARTBEAT_VMAJOR_length		2

#define GRID_CLASS_HEARTBEAT_VMINOR_offset		9
#define GRID_CLASS_HEARTBEAT_VMINOR_length		2

#define GRID_CLASS_HEARTBEAT_VPATCH_offset		11
#define GRID_CLASS_HEARTBEAT_VPATCH_length		2


// DEBUG
//#define GRID_CLASS_DEBUGTEXT_code			0x020
//#define GRID_CLASS_DEBUGTEXT_format			"%c%03x%01x%s%c",GRID_CONST_STX,GRID_CLASS_DEBUGTEXT_code,instruction,message,GRID_CONST_ETX

// BANK
#define GRID_CLASS_BANKACTIVE_code			0x030
#define GRID_CLASS_BANKACTIVE_format		"%c%03x%01x%02x%c",GRID_CONST_STX,GRID_CLASS_BANKACTIVE_code,instruction,banknumber,GRID_CONST_ETX
#define GRID_CLASS_BANKACTIVE_frame			"%c%03x_..%c",GRID_CONST_STX,GRID_CLASS_BANKACTIVE_code,GRID_CONST_ETX

#define GRID_CLASS_BANKACTIVE_BANKNUMBER_offset		5
#define GRID_CLASS_BANKACTIVE_BANKNUMBER_length		2



//#define GRID_CLASS_BANKENABLED_code		0x031
//#define GRID_CLASS_BANKENABLED_format		"%c%03x%01x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_BANKACTIVE_code,instruction,banknumber,isenabled,GRID_CONST_ETX	

//#define GRID_CLASS_BANKCOLOR_code			0x032
//#define GRID_CLASS_BANKCOLOR_format		"%c%03x%01x%02x%02x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_BANKACTIVE_code,instruction,banknumber,red,green,blue,GRID_CONST_ETX

#define GRID_CLASS_LEDPHASE_code			0x040
#define GRID_CLASS_LEDPHASE_format			"%c%03x%01x%02x%02x%02x%c",GRID_CONST_STX,GRID_CLASS_LEDPHASE_code,instruction,layernumber,lednumber,phase,GRID_CONST_ETX
#define GRID_CLASS_LEDPHASE_frame			"%c%03x_......%c",GRID_CONST_STX,GRID_CLASS_LEDPHASE_code,GRID_CONST_ETX

#define GRID_CLASS_LEDPHASE_LAYERNUMBER_offset		5
#define GRID_CLASS_LEDPHASE_LAYERNUMBER_length		2

#define GRID_CLASS_LEDPHASE_LEDNUMBER_offset		7
#define GRID_CLASS_LEDPHASE_LEDNUMBER_length		2

#define GRID_CLASS_LEDPHASE_PHASE_offset			9
#define GRID_CLASS_LEDPHASE_PHASE_length			2


#endif /* GRID_PROTOCOL_H_INCLUDED */
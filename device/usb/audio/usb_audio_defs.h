#ifndef USB_AUDIO_DEFS_H
#define USB_AUDIO_DEFS_H

// A.1 Audio Interface Class Code
#define AUDIO	0x01

// A.2 Audio Interface Subclass Codes
#define SUBCLASS_UNDEFINED	0x00
#define AUDIOCONTROL		0x01
#define AUDIOSTREAMING		0x02
#define MIDISTREAMING		0x03

// A.3 Audio Interface Protocol Codes
#define PR_PROTOCOL_UNDEFINED	0x00

// A.4 Audio Class-Specific Descriptor Types
#define CS_UNDEFINED		0x20
#define CS_DEVICE		0x21
#define CS_CONFIGURATION	0x22
#define CS_STRING		0x23
#define CS_INTERFACE		0x24
#define CS_ENDPOINT		0x25

// A.5 Audio Class-Specific AC Interface Descriptor Subtypes
#define AC_DESCRIPTOR_UNDEFINED	0x00
#define HEADER			0x01
#define INPUT_TERMINAL		0x02
#define OUTPUT_TERMINAL		0x03
#define MIXER_UNIT		0x04
#define SELECTOR_UNIT		0x05
#define FEATURE_UNIT		0x06
#define PROCESSING_UNIT		0x07
#define EXTENSION_UNIT		0x08

// A.6 Audio Class-Specific AS Interface Descriptor Subtypes
#define AS_DESCRIPTOR_UNDEFINED	0x00
#define AS_GENERAL		0x01
#define FORMAT_TYPE		0x02
#define FORMAT_SPECIFIC		0x03

// A.7 Processing Unit Process Types
#define PROCESS_UNDEFINED	0x00
#define UD_MIX_PROCESS		0x01
#define DOLBY_PROLOGIC_PROCESS	0x02
#define _3D_STEREO_EXTENDER_PROCESS	0x03
#define REVERBERATION_PROCESS	0x04
#define CHORUS_PROCESS		0x05
#define DYN_RANGE_COMP_PROCESS	0x06

// A.8 Audio Class-Specific Endpoint Descriptor Subtypes
#define DESCRIPTOR_UNDEFINED	0x00
#define EP_GENERAL		0x01

// A.9 Audio Class-Specific Request Codes
#define REQUEST_CODE_UNDEFINED	0x00
#define SET_CUR			0x01
#define GET_CUR			0x81
#define SET_MIN			0x02
#define GET_MIN			0x82
#define SET_MAX			0x03
#define GET_MAX			0x83
#define SET_RES			0x04
#define GET_RES			0x84
#define SET_MEM			0x05
#define GET_MEM			0x85
#define GET_STAT		0xff

// A.10.1 Terminal Control Selectors
#define TE_CONTROL_UNDEFINED	0x00
#define COPY_PROTECT_CONTROL	0x01

// A.10.2 Feature Unit Control Selectors
#define FU_CONTROL_UNDEFINED	0x00
#define MUTE_CONTROL		0x01
#define VOLUME_CONTROL		0x02
#define BASS_CONTROL		0x03
#define MID_CONTROL		0x04
#define TREBLE_CONTROL		0x05
#define GRAPHIC_EQUALIZER_CONTROL	0x06
#define AUTOMATIC_GAIN_CONTROL	0x07
#define DELAY_CONTROL		0x08
#define BASS_BOOST_CONTROL	0x09
#define LOUDNESS_CONTROL	0x0a

// A.10.3.1 Up/Down-mix Processing Unit Control Selectors
#define UD_CONTROL_UNDEFINED	0x00
#define UD_ENABLE_CONTROL	0x01
#define UD_MODE_SELECT_CONTROL	0x02

// A.10.3.2 Dolby Prologic Processing Unit Control Selectors
#define DP_CONTROL_UNDEFINED	0x00
#define DP_ENABLE_CONTROL	0x01
#define DP_MODE_SELECT_CONTROL	0x02

// A.10.3.3 3D Stereo Extender Processing Unit Control Selectors
#define _3D_CONTROL_UNDEFINED	0x00
#define _3D_ENABLE_CONTROL	0x01
#define SPACIOUSNESS_CONTROL	0x03

// A.10.3.4 Reverberation Processing Unit Control Selectors
#define RV_CONTROL_UNDEFINED	0x00
#define RV_ENABLE_CONTROL	0x01
#define REVERB_LEVEL_CONTROL	0x02
#define REVERB_TIME_CONTROL	0x03
#define REVERB_FEEDBACK_CONTROL	0x04

// A.10.3.5 Chorus Processing Unit Control Selectors
#define CH_CONTROL_UNDEFINED	0x00
#define CH_ENABLE_CONTROL	0x01
#define CHORUS_LEVEL_CONTROL	0x02
#define CHORUS_RATE_CONTROL	0x03
#define CHORUS_DEPTH_CONTROL	0x04

// A.10.3.6 Dynamic Range Compressor Processing Unit Control Selectors
#define DR_CONTROL_UNDEFINED	0x00
#define DR_ENABLE_CONTROL	0x01
#define COMPRESSION_RATE_CONTROL	0x02
#define MAXAMPL_CONTROL		0x03
#define THRESHOLD_CONTROL	0x04
#define ATTACK_TIME		0x05
#define RELEASE_TIME		0x06

// A.10.4 Extension Unit Control Selectors
#define XU_CONTROL_UNDEFINED	0x00
#define XU_ENABLE_CONTROL	0x01

// A.10.5 Endpoint Control Selectors
#define EP_CONTROL_UNDEFINED	0x00
#define SAMPLING_FREQ_CONTROL	0x01
#define PITCH_CONTROL		0x02

// Audio format type codec
#define FORMAT_TYPE_UNDEFINED	0x00
#define FORMAT_TYPE_I		0x01
#define FORMAT_TYPE_II		0x02
#define FORMAT_TYPE_III		0x03

// Audio data formats
enum {
	// Type I
	TYPE_I_UNDEFINED		= 0x0000,
	PCM				= 0x0001,
	PCM8				= 0x0002,
	IEEE_FLOAT			= 0x0003,
	ALAW				= 0x0004,
	MULAW				= 0x0005,
	// Type II
	TYPE_II_UNDEFINED		= 0x1000,
	MPEG				= 0x1001,
	AC_3				= 0x1002,
	// Type III
	TYPE_III_UNDEFINED		= 0x2000,
	IEC1937_AC_3			= 0x2001,
	IEC1937_MPEG_1_LAYER1		= 0x2002,
	IEC1937_MPEG_2_NOEXT		= 0x2003,
	IEC1937_MPEG_2_EXT		= 0x2004,
	IEC1937_MPEG_2_Layer1_LS	= 0x2005,
	IEC1937_MPEG_2_Layer2_3_LS	= 0x2006,
};

// Audio terminal types
enum {
	// USB terminal types
	USB_Undefined			= 0x0100,
	USB_streaming			= 0x0101,
	USB_vendor_specific		= 0x01FF,
	// Input terminal types
	Input_Undefined			= 0x0200,
	Microphone			= 0x0201,
	Desktop_microphone		= 0x0202,
	Personal_microphone		= 0x0203,
	Omni_directional_microphone	= 0x0204,
	Microphone_array		= 0x0205,
	Processing_microphone_array	= 0x0206,
	// Output terminal types
	Output_Undefined		= 0x0300,
	Speaker				= 0x0301,
	Headphones			= 0x0302,
	Head_Mounted_Display_Audio	= 0x0303,
	Desktop_speaker			= 0x0304,
	Room_speaker			= 0x0305,
	Communication_speaker		= 0x0306,
	Low_frequency_effects_speaker	= 0x0307,
};

#endif // USB_AUDIO_DEFS_H

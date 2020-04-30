#ifndef USB_AUDIO2_DEFS_TERMT_H
#define USB_AUDIO2_DEFS_TERMT_H

enum {
	// 2.1 USB Terminal Types
	USB_Undefined			= 0x0100,
	USB_streaming			= 0x0101,
	USB_vendor_specific		= 0x01FF,

	// 2.2 Input Terminal Types
	Input_Undefined			= 0x0200,
	Microphone			= 0x0201,
	Desktop_microphone		= 0x0202,
	Personal_microphone		= 0x0203,
	Omni_directional_microphone	= 0x0204,
	Microphone_array		= 0x0205,
	Processing_microphone_array	= 0x0206,

	// 2.3 Output Terminal Types
	Output_Undefined		= 0x0300,
	Speaker				= 0x0301,
	Headphones			= 0x0302,
	Head_Mounted_Display_Audio	= 0x0303,
	Desktop_speaker			= 0x0304,
	Room_speaker			= 0x0305,
	Communication_speaker		= 0x0306,
	Low_frequency_effects_speaker	= 0x0307,

	// 2.4 Bi-directional Terminal Types
	Bi_directional_Undefined	= 0x0400,
	Handset				= 0x0401,
	Headset				= 0x0402,
	Speakerphone_no_echo_reduction	= 0x0403,
	Echo_suppressing_speakerphone	= 0x0404,
	Echo_canceling_speakerphone	= 0x0405,

	// 2.5 Telephony Terminal Types
	Telephony_Undefined		= 0x0500,
	Phone_line			= 0x0501,
	Telephone			= 0x0502,
	Down_Line_Phone			= 0x0503,

	// 2.6 External Terminal Types
	External_Undefined		= 0x0600,
	Analog_connector		= 0x0601,
	Digital_audio_interface		= 0x0602,
	Line_connector			= 0x0603,
	Legacy_audio_connector		= 0x0604,
	S_PDIF_interface		= 0x0605,
	_1394_DA_stream			= 0x0606,
	_1394_DV_stream_soundtrack	= 0x0607,
	ADAT_Lightpipe			= 0x0608,
	TDIF				= 0x0609,
	MADI				= 0x060A,

	// 2.7 Embedded Function Terminal Types
	Embedded_Undefined		= 0x0700,
	Level_Calibration_Noise_Source	= 0x0701,
	Equalization_Noise		= 0x0702,
	CD_player			= 0x0703,
	DAT				= 0x0704,
	DCC				= 0x0705,
	Compressed_Audio_Player		= 0x0706,
	Analog_Tape			= 0x0707,
	Phonograph			= 0x0708,
	VCR_Audio			= 0x0709,
	Video_Disc_Audio		= 0x070A,
	DVD_Audio			= 0x070B,
	TV_Tuner_Audio			= 0x070C,
	Satellite_Receiver_Audio	= 0x070D,
	Cable_Tuner_Audio		= 0x070E,
	DSS_Audio			= 0x070F,
	Radio_Receiver			= 0x0710,
	Radio_Transmitter		= 0x0711,
	Multi_track_Recorder		= 0x0712,
	Synthesizer			= 0x0713,
	Piano				= 0x0714,
	Guitar				= 0x0715,
	Drums_Rhythm			= 0x0716,
	Other_Musical_Instrument	= 0x0717,
};

#endif // USB_AUDIO2_DEFS_TERMT_H

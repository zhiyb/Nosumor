#ifndef USB_AUDIO2_DEFS_FRMTS_H
#define USB_AUDIO2_DEFS_FRMTS_H

// A.1 Format Type Codes
#define FORMAT_TYPE_UNDEFINED	0x00
#define FORMAT_TYPE_I		0x01
#define FORMAT_TYPE_II		0x02
#define FORMAT_TYPE_III		0x03
#define FORMAT_TYPE_IV		0x04
#define EXT_FORMAT_TYPE_I	0x81
#define EXT_FORMAT_TYPE_II	0x82
#define EXT_FORMAT_TYPE_III	0x83

// A.2 Audio Data Format Bit Allocation in the bmFormats field

// A.2.1 Audio Data Format Type I Bit Allocations
#define FRMT_I_PCM				0
#define FRMT_I_PCM8				1
#define FRMT_I_IEEE_FLOAT			2
#define FRMT_I_ALAW				3
#define FRMT_I_MULAW				4
#define FRMT_I_TYPE_I_RAW_DATA			31

// A.2.2 Audio Data Format Type II Bit Allocations
#define FRMT_II_MPEG				0
#define FRMT_II_AC_3				1
#define FRMT_II_WMA				2
#define FRMT_II_DTS				3
#define FRMT_II_TYPE_II_RAW_DATA		31

// A.2.3 Audio Data Format Type III Bit Allocations
#define FRMT_III_IEC61937_AC_3			0
#define FRMT_III_IEC61937_MPEG_1_Layer1		1
#define FRMT_III_IEC61937_MPEG_1_Layer2_3	2
#define FRMT_III_IEC61937_MPEG_2_NOEXT		2
#define FRMT_III_IEC61937_MPEG_2_EXT		3
#define FRMT_III_IEC61937_MPEG_2_AAC_ADTS	4
#define FRMT_III_IEC61937_MPEG_2_Layer1_LS	5
#define FRMT_III_IEC61937_MPEG_2_Layer2_3_LS	6
#define FRMT_III_IEC61937_DTS_I			7
#define FRMT_III_IEC61937_DTS_II		8
#define FRMT_III_IEC61937_DTS_III		9
#define FRMT_III_IEC61937_ATRAC			10
#define FRMT_III_IEC61937_ATRAC2_3		11
#define FRMT_III_TYPE_III_WMA			12

// A.2.4 Audio Data Format Type IV Bit Allocations
#define FRMT_IV_PCM				0
#define FRMT_IV_PCM8				1
#define FRMT_IV_IEEE_FLOAT			2
#define FRMT_IV_ALAW				3
#define FRMT_IV_MULAW				4
#define FRMT_IV_MPEG				5
#define FRMT_IV_AC_3				6
#define FRMT_IV_WMA				7
#define FRMT_IV_IEC61937_AC_3			8
#define FRMT_IV_IEC61937_MPEG_1_Layer1		9
#define FRMT_IV_IEC61937_MPEG_1_Layer2_3	10
#define FRMT_IV_IEC61937_MPEG_2_NOEXT		10
#define FRMT_IV_IEC61937_MPEG_2_EXT		11
#define FRMT_IV_IEC61937_MPEG_2_AAC_ADTS	12
#define FRMT_IV_IEC61937_MPEG_2_Layer1_LS	13
#define FRMT_IV_IEC61937_MPEG_2_Layer2_3_LS	14
#define FRMT_IV_IEC61937_DTS_I			15
#define FRMT_IV_IEC61937_DTS_II			16
#define FRMT_IV_IEC61937_DTS_III		17
#define FRMT_IV_IEC61937_ATRAC			18
#define FRMT_IV_IEC61937_ATRAC2_3		19
#define FRMT_IV_TYPE_III_WMA			20
#define FRMT_IV_IEC60958_PCM			21

// A.3 Side Band Protocol Codes
#define PROTOCOL_UNDEFINED	0x00
#define PRES_TIMESTAMP_PROTOCOL	0x01

#endif // USB_AUDIO2_DEFS_FRMTS_H

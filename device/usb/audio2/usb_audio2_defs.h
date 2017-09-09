#ifndef USB_AUDIO2_DEFS_H
#define USB_AUDIO2_DEFS_H

// A.1 Audio Function Class Code
#define AUDIO_FUNCTION			AUDIO

// A.2 Audio Function Subclass Codes
#define FUNCTION_SUBCLASS_UNDEFINED	0x00

// A.3 Audio Function Protocol Codes
#define FUNCTION_PROTOCOL_UNDEFINED	0x00
#define AF_VERSION_02_00		IP_VERSION_02_00

// A.4 Audio Interface Class Code
#define AUDIO				0x01

// A.5 Audio Interface Subclass Codes
#define INTERFACE_SUBCLASS_UNDEFINED	0x00
#define AUDIOCONTROL			0x01
#define AUDIOSTREAMING			0x02
#define MIDISTREAMING			0x03

// A.6 Audio Interface Protocol Codes
#define INTERFACE_PROTOCOL_UNDEFINED	0x00
#define IP_VERSION_02_00		0x20

// A.7 Audio Function Category Codes
#define FUNCTION_SUBCLASS_UNDEFINED	0x00
#define DESKTOP_SPEAKER			0x01
#define HOME_THEATER			0x02
#define MICROPHONE			0x03
#define HEADSET				0x04
#define TELEPHONE			0x05
#define CONVERTER			0x06
#define VOICE_SOUND_RECORDER		0x07
#define I_O_BOX				0x08
#define MUSICAL_INSTRUMENT		0x09
#define PRO_AUDIO			0x0A
#define AUDIO_VIDEO			0x0B
#define CONTROL_PANEL			0x0C
#define OTHER				0xFF

// A.8 Audio Class-Specific Descriptor Types
#define CS_UNDEFINED			0x20
#define CS_DEVICE			0x21
#define CS_CONFIGURATION		0x22
#define CS_STRING			0x23
#define CS_INTERFACE			0x24
#define CS_ENDPOINT			0x25

// A.9 Audio Class-Specific AC Interface Descriptor Subtypes
#define AC_DESCRIPTOR_UNDEFINED		0x00
#define HEADER				0x01
#define INPUT_TERMINAL			0x02
#define OUTPUT_TERMINAL			0x03
#define MIXER_UNIT			0x04
#define SELECTOR_UNIT			0x05
#define FEATURE_UNIT			0x06
#define EFFECT_UNIT			0x07
#define PROCESSING_UNIT			0x08
#define EXTENSION_UNIT			0x09
#define CLOCK_SOURCE			0x0A
#define CLOCK_SELECTOR			0x0B
#define CLOCK_MULTIPLIER		0x0C
#define SAMPLE_RATE_CONVERTER		0x0D

// A.10 Audio Class-Specific AS Interface Descriptor Subtypes
#define AS_DESCRIPTOR_UNDEFINED		0x00
#define AS_GENERAL			0x01
#define FORMAT_TYPE			0x02
#define ENCODER				0x03
#define DECODER				0x04

// A.11 Effect Unit Effect Types
#define EFFECT_UNDEFINED		0x00
#define PARAM_EQ_SECTION_EFFECT		0x01
#define REVERBERATION_EFFECT		0x02
#define MOD_DELAY_EFFECT		0x03
#define DYN_RANGE_COMP_EFFECT		0x04

// A.12 Processing Unit Process Types
#define PROCESS_UNDEFINED		0x00
#define UP_DOWNMIX_PROCESS		0x01
#define DOLBY_PROLOGIC_PROCESS		0x02
#define STEREO_EXTENDER_PROCESS		0x03

// A.13 Audio Class-Specific Endpoint Descriptor Subtypes
#define DESCRIPTOR_UNDEFINED		0x00
#define EP_GENERAL			0x01

// A.14 Audio Class-Specific Request Codes
#define REQUEST_CODE_UNDEFINED		0x00
#define CUR				0x01
#define RANGE				0x02
#define MEM				0x03

// A.15 Encoder Type Codes
#define ENCODER_UNDEFINED		0x00
#define OTHER_ENCODER			0x01
#define MPEG_ENCODER			0x02
#define AC_3_ENCODER			0x03
#define WMA_ENCODER			0x04
#define DTS_ENCODER			0x05

// A.16 Decoder Type Codes
#define DECODER_UNDEFINED		0x00
#define OTHER_DECODER			0x01
#define MPEG_DECODER			0x02
#define AC_3_DECODER			0x03
#define WMA_DECODER			0x04
#define DTS_DECODER			0x05

// A.17 Control Selector Codes

// A.17.1 Clock Source Control Selectors
#define CS_CONTROL_UNDEFINED		0x00
#define CS_SAM_FREQ_CONTROL		0x01
#define CS_CLOCK_VALID_CONTROL		0x02

// A.17.2 Clock Selector Control Selectors
#define CX_CONTROL_UNDEFINED		0x00
#define CX_CLOCK_SELECTOR_CONTROL	0x01

// A.17.3 Clock Multiplier Control Selectors
#define CM_CONTROL_UNDEFINED		0x00
#define CM_NUMERATOR_CONTROL		0x01
#define CM_DENOMINATOR_CONTROL		0x02

// A.17.4 Terminal Control Selectors
#define TE_CONTROL_UNDEFINED		0x00
#define TE_COPY_PROTECT_CONTROL		0x01
#define TE_CONNECTOR_CONTROL		0x02
#define TE_OVERLOAD_CONTROL		0x03
#define TE_CLUSTER_CONTROL		0x04
#define TE_UNDERFLOW_CONTROL		0x05
#define TE_OVERFLOW_CONTROL		0x06
#define TE_LATENCY_CONTROL		0x07

// A.17.5 Mixer Control Selectors
#define MU_CONTROL_UNDEFINED		0x00
#define MU_MIXER_CONTROL		0x01
#define MU_CLUSTER_CONTROL		0x02
#define MU_UNDERFLOW_CONTROL		0x03
#define MU_OVERFLOW_CONTROL		0x04
#define MU_LATENCY_CONTROL		0x05

// A.17.6 Selector Control Selectors
#define SU_CONTROL_UNDEFINED		0x00
#define SU_SELECTOR_CONTROL		0x01
#define SU_LATENCY_CONTROL		0x02

// A.17.7 Feature Unit Control Selectors
#define FU_CONTROL_UNDEFINED		0x00
#define FU_MUTE_CONTROL			0x01
#define FU_VOLUME_CONTROL		0x02
#define FU_BASS_CONTROL			0x03
#define FU_MID_CONTROL			0x04
#define FU_TREBLE_CONTROL		0x05
#define FU_GRAPHIC_EQUALIZER_CONTROL	0x06
#define FU_AUTOMATIC_GAIN_CONTROL	0x07
#define FU_DELAY_CONTROL		0x08
#define FU_BASS_BOOST_CONTROL		0x09
#define FU_LOUDNESS_CONTROL		0x0A
#define FU_INPUT_GAIN_CONTROL		0x0B
#define FU_INPUT_GAIN_PAD_CONTROL	0x0C
#define FU_PHASE_INVERTER_CONTROL	0x0D
#define FU_UNDERFLOW_CONTROL		0x0E
#define FU_OVERFLOW_CONTROL		0x0F
#define FU_LATENCY_CONTROL		0x10

// A.17.8 Effect Unit Control Selectors

// A.17.8.1 Parametric Equalizer Section Effect Unit Control Selectors
#define PE_CONTROL_UNDEFINED		0x00
#define PE_ENABLE_CONTROL		0x01
#define PE_CENTERFREQ_CONTROL		0x02
#define PE_QFACTOR_CONTROL		0x03
#define PE_GAIN_CONTROL			0x04
#define PE_UNDERFLOW_CONTROL		0x05
#define PE_OVERFLOW_CONTROL		0x06
#define PE_LATENCY_CONTROL		0x07

// A.17.8.2 Reverberation Effect Unit Control Selectors
#define RV_CONTROL_UNDEFINED		0x00
#define RV_ENABLE_CONTROL		0x01
#define RV_TYPE_CONTROL			0x02
#define RV_LEVEL_CONTROL		0x03
#define RV_TIME_CONTROL			0x04
#define RV_FEEDBACK_CONTROL		0x05
#define RV_PREDELAY_CONTROL		0x06
#define RV_DENSITY_CONTROL		0x07
#define RV_HIFREQ_ROLLOFF_CONTROL	0x08
#define RV_UNDERFLOW_CONTROL		0x09
#define RV_OVERFLOW_CONTROL		0x0A
#define RV_LATENCY_CONTROL		0x0B

// A.17.8.3 Modulation Delay Effect Unit Control Selectors
#define MD_CONTROL_UNDEFINED		0x00
#define MD_ENABLE_CONTROL		0x01
#define MD_BALANCE_CONTROL		0x02
#define MD_RATE_CONTROL			0x03
#define MD_DEPTH_CONTROL		0x04
#define MD_TIME_CONTROL			0x05
#define MD_FEEDBACK_CONTROL		0x06
#define MD_UNDERFLOW_CONTROL		0x07
#define MD_OVERFLOW_CONTROL		0x08
#define MD_LATENCY_CONTROL		0x09

// A.17.8.4 Dynamic Range Compressor Effect Unit Control Selectors
#define DR_CONTROL_UNDEFINED		0x00
#define DR_ENABLE_CONTROL		0x01
#define DR_COMPRESSION_RATE_CONTROL	0x02
#define DR_MAXAMPL_CONTROL		0x03
#define DR_THRESHOLD_CONTROL		0x04
#define DR_ATTACK_TIME_CONTROL		0x05
#define DR_RELEASE_TIME_CONTROL		0x06
#define DR_UNDERFLOW_CONTROL		0x07
#define DR_OVERFLOW_CONTROL		0x08
#define DR_LATENCY_CONTROL		0x09

// A.17.9 Processing Unit Control Selectors

// A.17.9.1 Up/Down-mix Processing Unit Control Selectors
#define UD_CONTROL_UNDEFINED		0x00
#define UD_ENABLE_CONTROL		0x01
#define UD_MODE_SELECT_CONTROL		0x02
#define UD_CLUSTER_CONTROL		0x03
#define UD_UNDERFLOW_CONTROL		0x04
#define UD_OVERFLOW_CONTROL		0x05
#define UD_LATENCY_CONTROL		0x06

// A.17.9.2 Dolby Prologic™ Processing Unit Control Selectors
#define DP_CONTROL_UNDEFINED		0x00
#define DP_ENABLE_CONTROL		0x01
#define DP_MODE_SELECT_CONTROL		0x02
#define DP_CLUSTER_CONTROL		0x03
#define DP_UNDERFLOW_CONTROL		0x04
#define DP_OVERFLOW_CONTROL		0x05
#define DP_LATENCY_CONTROL		0x06

// A.17.9.3 Stereo Extender Processing Unit Control Selectors
#define ST_EXT_CONTROL_UNDEFINED	0x00
#define ST_EXT_ENABLE_CONTROL		0x01
#define ST_EXT_WIDTH_CONTROL		0x02
#define ST_EXT_UNDERFLOW_CONTROL	0x03
#define ST_EXT_OVERFLOW_CONTROL		0x04
#define ST_EXT_LATENCY_CONTROL		0x05

// A.17.10 Extension Unit Control Selectors
#define XU_CONTROL_UNDEFINED		0x00
#define XU_ENABLE_CONTROL		0x01
#define XU_CLUSTER_CONTROL		0x02
#define XU_UNDERFLOW_CONTROL		0x03
#define XU_OVERFLOW_CONTROL		0x04
#define XU_LATENCY_CONTROL		0x05

// A.17.11 AudioStreaming Interface Control Selectors
#define AS_CONTROL_UNDEFINED		0x00
#define AS_ACT_ALT_SETTING_CONTROL	0x01
#define AS_VAL_ALT_SETTINGS_CONTROL	0x02
#define AS_AUDIO_DATA_FORMAT_CONTROL	0x03

// A.17.12 Encoder Control Selectors
#define EN_CONTROL_UNDEFINED		0x00
#define EN_BIT_RATE_CONTROL		0x01
#define EN_QUALITY_CONTROL		0x02
#define EN_VBR_CONTROL			0x03
#define EN_TYPE_CONTROL			0x04
#define EN_UNDERFLOW_CONTROL		0x05
#define EN_OVERFLOW_CONTROL		0x06
#define EN_ENCODER_ERROR_CONTROL	0x07
#define EN_PARAM1_CONTROL		0x08
#define EN_PARAM2_CONTROL		0x09
#define EN_PARAM3_CONTROL		0x0A
#define EN_PARAM4_CONTROL		0x0B
#define EN_PARAM5_CONTROL		0x0C
#define EN_PARAM6_CONTROL		0x0D
#define EN_PARAM7_CONTROL		0x0E
#define EN_PARAM8_CONTROL		0x0F

// A.17.13 Decoder Control Selectors

// A.17.13.1 MPEG Decoder Control Selectors
#define MD_CONTROL_UNDEFINED		0x00
#define MD_DUAL_CHANNEL_CONTROL		0x01
#define MD_SECOND_STEREO_CONTROL	0x02
#define MD_MULTILINGUAL_CONTROL		0x03
#define MD_DYN_RANGE_CONTROL		0x04
#define MD_SCALING_CONTROL		0x05
#define MD_HILO_SCALING_CONTROL		0x06
#define MD_UNDERFLOW_CONTROL		0x07
#define MD_OVERFLOW_CONTROL		0x08
#define MD_DECODER_ERROR_CONTROL	0x09

// A.17.13.2 AC-3 Decoder Control Selectors
#define AD_CONTROL_UNDEFINED		0x00
#define AD_MODE_CONTROL			0x01
#define AD_DYN_RANGE_CONTROL		0x02
#define AD_SCALING_CONTROL		0x03
#define AD_HILO_SCALING_CONTROL		0x04
#define AD_UNDERFLOW_CONTROL		0x05
#define AD_OVERFLOW_CONTROL		0x06
#define AD_DECODER_ERROR_CONTROL	0x07

// A.17.13.3 WMA Decoder Control Selectors
#define WD_CONTROL_UNDEFINED		0x00
#define WD_UNDERFLOW_CONTROL		0x01
#define WD_OVERFLOW_CONTROL		0x02
#define WD_DECODER_ERROR_CONTROL	0x03

// A.17.13.4 DTS Decoder Control Selectors
#define DD_CONTROL_UNDEFINED		0x00
#define DD_UNDERFLOW_CONTROL		0x01
#define DD_OVERFLOW_CONTROL		0x02
#define DD_DECODER_ERROR_CONTROL	0x03

// A.17.14 Endpoint Control Selectors
#define EP_CONTROL_UNDEFINED		0x00
#define EP_PITCH_CONTROL		0x01
#define EP_DATA_OVERRUN_CONTROL		0x02
#define EP_DATA_UNDERRUN_CONTROL	0x03

#define BIT(b)	(1ul << (b))

#include "usb_audio2_defs_spatial.h"
#include "usb_audio2_defs_frmts.h"
#include "usb_audio2_defs_termt.h"

// bmControls
#define BM_NA	0b00
#define BM_RO	0b01
#define BM_RW	0b11

// Feature Unit bmaControls
// f: A.17.7 Feature Unit Control Selectors
// e: bmControls
#define FU_CTRL(f, e)	((e) << (((f) - 1u) << 1u))

#endif // USB_AUDIO2_DEFS_H

#ifndef USB_AUDIO_DESC_H
#define USB_AUDIO_DESC_H

#include <stdint.h>
#include "usb_audio_defs.h"
#include "../usb_macros.h"
#include "../usb.h"

// Audio control interface
typedef struct PACKED desc_ac_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdADC;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t baInterfaceNr[];
} desc_ac_t;

static desc_ac_t desc_ac = {
	8u + 1u, CS_INTERFACE, HEADER, 0x0100u, 0u, 1u, {1u},
};

// Units
enum {
	IT_USB = 1,
	FU_Out,
	OT_Speaker,
};

// Input terminal
typedef struct PACKED desc_it_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bNrChannels;
	uint16_t wChannelConfig;
	uint8_t iChannelNames;
	uint8_t iTerminal;
} desc_it_t;

static const desc_it_t desc_itd[] = {
	{12u, CS_INTERFACE, INPUT_TERMINAL, IT_USB, USB_streaming, 0u, 2u, 0b11, 0u, 0u},
};

// Output terminal
typedef struct PACKED desc_ot_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
} desc_ot_t;

static const desc_ot_t desc_otd[] = {
	{9u, CS_INTERFACE, OUTPUT_TERMINAL, OT_Speaker, Speaker, 0u, FU_Out, 0u},
};

// Feature unit
typedef struct PACKED desc_fu_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t bControlSize;
	uint8_t bmaControls[];
	//uint8_t iFeature;
} desc_fu_t;

static const desc_fu_t desc_fu_out = {
	7u + 2u, CS_INTERFACE, FEATURE_UNIT, FU_Out, IT_USB, 1u, {0b11, 0b11, 0u}
};

// Type I format descriptor
typedef struct PACKED desc_type_i_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bNrChannels;
	uint8_t bSubframeSize;
	uint8_t bBitResolution;
	uint8_t bSamFreqType;
	uint8_t tSamFreq[3];
} desc_type_i_t;

static const desc_type_i_t desc_pcm[] = {
	{8u + 3u * 1u, CS_INTERFACE, FORMAT_TYPE, FORMAT_TYPE_I, 2u, 4u, 24u,
	1u, {0x00, 0xee, 0x02}},	// 192,000
	//1u, {0x80, 0xbb, 0x00}},	// 48,000
	//1u, {0x44, 0xac, 0x00}},	// 44,100
	{8u + 3u * 1u, CS_INTERFACE, FORMAT_TYPE, FORMAT_TYPE_I, 2u, 4u, 24u,
	//1u, {{0x00, 0xee, 0x02}}	// 192,000
	1u, {0x80, 0xbb, 0x00}},	// 48,000
	//1u, {0x44, 0xac, 0x00}},	// 44,100
	{8u + 3u * 1u, CS_INTERFACE, FORMAT_TYPE, FORMAT_TYPE_I, 2u, 2u, 16u,
	//1u, {{0x00, 0xee, 0x02}}	// 192,000
	1u, {0x80, 0xbb, 0x00}},	// 48,000
	//1u, {0x44, 0xac, 0x00}},	// 44,100
};

// Audio stream interface
typedef struct PACKED desc_as_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalLink;
	uint8_t bDelay;
	uint16_t wFormatTag;
} desc_as_t;

static const desc_as_t desc_as[] = {
	{7u, CS_INTERFACE, AS_GENERAL, IT_USB, 1u, PCM},
};

// Class specific endpoint descriptor
typedef struct PACKED desc_ep_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmAttributes;
	uint8_t bLockDelayUnits;
	uint8_t wLockDelay;
} desc_ep_t;

static const desc_ep_t desc_ep[] = {
	{7u, CS_ENDPOINT, EP_GENERAL, 0u, 0u, 0u},
};

#endif // USB_AUDIO_DESC_H

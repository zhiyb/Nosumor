#ifndef USB_AUDIO2_DESC_H
#define USB_AUDIO2_DESC_H

#include <stdint.h>
#include "usb_audio2_defs.h"
#include "../usb_macros.h"
#include "../usb.h"

// Audio control interface
typedef struct PACKED desc_ac_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdADC;
	uint8_t bCategory;
	uint16_t wTotalLength;
	uint8_t bmControls;
} desc_ac_t;

static desc_ac_t desc_ac = {
	9u, CS_INTERFACE, HEADER, 0x0200u, DESKTOP_SPEAKER, 0u, 0u,
};

// Units
enum {
	CS_USB = 1,
	IT_USB,
	FU_Out,
	OT_Speaker,
};

// Clock source
typedef struct PACKED desc_csd_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bClockID;
	// D1..0: Clock Type
	//    00: External Clock
	//    01: Internal fixed Clock
	//    10: Internal variable Clock
	//    11: Internal programmable Clock
	// D2   : Clock synchronized to SOF
	uint8_t bmAttributes;
	// D1..0: Clock Frequency Control
	// D3..2: Clock Validity Control
	uint8_t bmControls;
	uint8_t bAssocTerminal;
	uint8_t iClockSource;
} desc_csd_t;

static const desc_csd_t desc_csd[] = {
	{8u, CS_INTERFACE, CLOCK_SOURCE, CS_USB, 0b101, 0b0001u, IT_USB, 0u},
};

// Input terminal
typedef struct PACKED desc_itd_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bCSourceID;
	uint8_t bNrChannels;
	uint32_t bmChannelConfig;
	uint8_t iChannelNames;
	uint16_t bmControls;
	uint8_t iTerminal;
} desc_itd_t;

static const desc_itd_t desc_itd[] = {
	{17u, CS_INTERFACE, INPUT_TERMINAL, IT_USB, USB_streaming, 0u, CS_USB,
	 2u, SP_FL | SP_FR, 0u, 0u, 0u},
};

// Output terminal
typedef struct PACKED desc_otd_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t bCSourceID;
	uint16_t bmControls;
	uint8_t iTerminal;
} desc_otd_t;

static const desc_otd_t desc_otd[] = {
	{12u, CS_INTERFACE, OUTPUT_TERMINAL, OT_Speaker, Speaker, 0u, FU_Out, CS_USB, 0u, 0u},
};

// Feature unit
typedef struct PACKED desc_fu_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint32_t bmaControls[];
	//uint8_t iFeature;
} desc_fu_t;

static const desc_fu_t desc_fu_out = {
	6u + (2u + 1u) * 4u, CS_INTERFACE, FEATURE_UNIT, FU_Out, IT_USB,
	{FU_CTRL(FU_MUTE_CONTROL, BM_RW) | FU_CTRL(FU_VOLUME_CONTROL, BM_RW),
	 FU_CTRL(FU_MUTE_CONTROL, BM_RW) | FU_CTRL(FU_VOLUME_CONTROL, BM_RW),
	 FU_CTRL(FU_MUTE_CONTROL, BM_RW) | FU_CTRL(FU_VOLUME_CONTROL, BM_RW), 0u}
};

// Audio stream interface
typedef struct PACKED desc_as_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalLink;
	uint8_t bmControls;
	uint8_t bFormatType;
	uint32_t bmFormats;
	uint8_t bNrChannels;
	uint32_t bmChannelConfig;
	uint8_t iChannelNames;
} desc_as_t;

static const desc_as_t desc_as[] = {
	{16u, CS_INTERFACE, AS_GENERAL, IT_USB, 0u,
	 FORMAT_TYPE_I, BIT(FRMT_I_PCM), 2u, SP_FL | SP_FR, 0u},
};

// Class specific endpoint descriptor
typedef struct PACKED desc_ep_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmAttributes;
	uint8_t bmControls;
	uint8_t bLockDelayUnits;
	uint8_t wLockDelay;
} desc_ep_t;

static const desc_ep_t desc_ep[] = {
	{8u, CS_ENDPOINT, EP_GENERAL, 0u, 0u, 1u, 0u},
};

// Type I format descriptor
typedef struct PACKED desc_type_i_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bSubslotSize;
	uint8_t bBitResolution;
} desc_type_i_t;

static const desc_type_i_t desc_pcm[] = {
	{6u, CS_INTERFACE, FORMAT_TYPE, FORMAT_TYPE_I, 4u, 32u},
};

#endif // USB_AUDIO2_DESC_H

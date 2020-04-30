#ifndef USB_AUDIO2_DESC_H
#define USB_AUDIO2_DESC_H

#include <stdint.h>
#include "usb_audio2_defs.h"
#include "../usb_macros.h"
#include "../usb.h"

// Audio channel cluster
typedef struct PACKED desc_acc_t {
	uint8_t bNrChannels;
	uint32_t bmChannelConfig;
	uint8_t iChannelNames;
} desc_acc_t;

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

static const desc_ac_t desc_ac = {
	9u, CS_INTERFACE, HEADER, 0x0200u, OTHER, 0u, 0u,
};

// Clock source
typedef struct PACKED desc_cs_t {
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
} desc_cs_t;

#define CS_EXTERNAL	0b000
#define CS_FIXED	0b001
#define CS_VARIABLE	0b010
#define CS_PROGRAMMABLE	0b011
#define CS_SOF		0b100

// Clock selector
typedef struct PACKED desc_cx_t {
	uint8_t bLength;		// 7 + p
	uint8_t bDescriptorType;	// CS_INTERFACE
	uint8_t bDescriptorSubtype;	// CLOCK_SELECTOR
	uint8_t bClockID;
	uint8_t bNrInPins;		// p
	uint8_t baCSourceID[];		// 1 ... p
	// D1..0: Clock Selector Control
	//uint8_t bmControls;
	//uint8_t iClockSelector;
} desc_cx_t;

// Input terminal
typedef struct PACKED desc_it_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bCSourceID;
	desc_acc_t acc;
	uint16_t bmControls;
	uint8_t iTerminal;
} desc_it_t;

// Output terminal
typedef struct PACKED desc_ot_t {
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
} desc_ot_t;

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

// Audio stream interface
typedef struct PACKED desc_as_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalLink;
	uint8_t bmControls;
	uint8_t bFormatType;
	uint32_t bmFormats;
	desc_acc_t acc;
} desc_as_t;

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
	{8u, CS_ENDPOINT, EP_GENERAL, 0u, 0u, 0u, 0u},
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


// Parameter block layout types
typedef uint8_t layout1_cur_t;
typedef struct PACKED {
	uint8_t min, max, res;
} layout1_range_t;

typedef uint16_t layout2_cur_t;
typedef struct PACKED {
	uint16_t min, max, res;
} layout2_range_t;

typedef uint32_t layout3_cur_t;
typedef struct PACKED {
	uint32_t min, max, res;
} layout3_range_t;

#endif // USB_AUDIO2_DESC_H

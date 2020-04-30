#include <string.h>
#include <usb/usb_desc.h>
#include <usb/audio2/usb_audio2.h>
#include <usb/audio2/usb_audio2_desc.h>
#include "audio.h"

USB_DESC_STRING_LIST();
USB_AUDIO2_ENTITY_LIST();

// Clock Source
static uint8_t ae_cs_desc(void *p, uint32_t size);
static uint32_t ae_cs_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
static uint32_t ae_cs_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
USB_AUDIO2_ENTITY(ae_cs) = {&ae_cs_desc, &ae_cs_get, &ae_cs_set};
USB_DESC_STRING(ae_cs_str) = {0, LANG_EN_US, "Audio Clock Source - PLL"};

// Feature Unit
static uint8_t ae_fu_desc(void *p, uint32_t size);
static uint32_t ae_fu_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
static uint32_t ae_fu_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
USB_AUDIO2_ENTITY(ae_fu) = {&ae_fu_desc, &ae_fu_get, &ae_fu_set};
USB_DESC_STRING(ae_fu_str) = {0, LANG_EN_US, "Audio Feature Unit - Volume & Mute"};

// Input Terminal
static uint8_t ae_it_usb_desc(void *p, uint32_t size);
static uint32_t ae_it_usb_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
static uint32_t ae_it_usb_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
USB_AUDIO2_ENTITY(ae_it_usb) = {&ae_it_usb_desc, &ae_it_usb_get, &ae_it_usb_set};
USB_DESC_STRING(ae_it_usb_str) = {0, LANG_EN_US, "Audio Input Terminal - USB"};

// Output Terminal
static uint8_t ae_ot_hp_desc(void *p, uint32_t size);
static uint32_t ae_ot_hp_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
static uint32_t ae_ot_hp_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length);
USB_AUDIO2_ENTITY(ae_ot_hp) = {&ae_ot_hp_desc, &ae_ot_hp_get, &ae_ot_hp_set};
USB_DESC_STRING(ae_ot_hp_str) = {0, LANG_EN_US, "Audio Output Terminal - Headphone Jack"};

// Clock Source
static uint8_t ae_cs_desc(void *p, uint32_t size)
{
	const desc_cs_t desc = {
		.bLength = 8,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = CLOCK_SOURCE,
		.bClockID = USB_AUDIO2_ENTITY_ID(ae_cs),
		.bmAttributes = 0b011,	// Programmable internal clock
		.bmControls = 0b0011,	// No validity control, writable frequency control
		.bAssocTerminal = 0,
		.iClockSource = USB_DESC_STRING_INDEX(ae_cs_str),
	};
#if DEBUG >= 5
	if (desc.bLength > size)
		USB_ERROR();
#endif
	*(desc_cs_t *)p = desc;
	return desc.bLength;
}

static uint32_t ae_cs_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	static const struct PACKED {
		uint16_t wNumSubRanges;
		layout3_range_t range[2];
	} freq_range = {2,
		{{192000, 192000, 0},
		 {44100, 44100, 0}},
	};

	switch (cs) {
	case CS_SAM_FREQ_CONTROL:	// Layout 3 parameter block
		if (request == CUR) {
			layout3_cur_t freq = audio_get_freq();
			*(layout3_cur_t *)p = freq;
			return MIN(length, sizeof(layout3_cur_t));
		} else if (request == RANGE) {
			if (cn != 0)
				break;
			memcpy(p, &freq_range, sizeof(freq_range));
			return MIN(length, sizeof(freq_range));
		}
	}
	USB_TODO();
	return -1;
}

static uint32_t ae_cs_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	switch (cs) {
	case CS_SAM_FREQ_CONTROL:	// Layout 3 parameter block
		if (request == CUR) {
			if (size != sizeof(layout3_cur_t))
				return -1;
			layout3_cur_t freq = *(layout3_cur_t *)p;
			return audio_set_freq(freq);
		}
	}
	USB_TODO();
	return -1;
}

// Feature Unit
static uint8_t ae_fu_desc(void *p, uint32_t size)
{
	const desc_fu_t desc = {
		.bLength = 5 + 3 * 4 + 1,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = FEATURE_UNIT,
		.bUnitID = USB_AUDIO2_ENTITY_ID(ae_fu),
		.bSourceID = USB_AUDIO2_ENTITY_ID(ae_it_usb),
		//uint32_t bmaControls[];
		//uint8_t iFeature;
	};
#if DEBUG >= 5
	if (desc.bLength > size)
		USB_ERROR();
#endif
	*(desc_fu_t *)p = desc;
	//uint32_t bmaControls[];
	*((uint32_t *)(p + 5) + 0) = 0b1111;	// Master channel, volume and mute
	*((uint32_t *)(p + 5) + 1) = 0b1111;	// Channel 1, volume and mute
	*((uint32_t *)(p + 5) + 2) = 0b1111;	// Channel 2, volume and mute
	//uint8_t iFeature;
	*(uint8_t *)(p + 5 + 3 * 4) = USB_DESC_STRING_INDEX(ae_fu_str);
	return desc.bLength;
}

static uint32_t ae_fu_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	static const struct PACKED {
		uint16_t wNumSubRanges;
		layout2_range_t range[1];
	} vol_range[3] = {
		{1, {{1, 100, 1}}},
		{1, {{1, 100, 1}}},
		{1, {{1, 100, 1}}},
	};

	switch (cs) {
	case FU_MUTE_CONTROL:		// Layout 1 parameter block
		if (request == CUR) {
			layout1_cur_t mute = audio_get_volume() == 0;
			*(layout1_cur_t *)p = mute;
			return MIN(length, sizeof(layout1_cur_t));
		}
		break;
	case FU_VOLUME_CONTROL:		// Layout 2 parameter block
		if (request == CUR) {
			layout2_cur_t vol = audio_get_volume();
			if (vol == 0)
				vol = 0x8000;	// Silence
			*(layout2_cur_t *)p = vol;
			return MIN(length, sizeof(layout2_cur_t));
		} else if (request == RANGE) {
			if (cn >= ASIZE(vol_range))
				break;
			memcpy(p, &vol_range[cn], sizeof(vol_range[0]));
			return MIN(length, 2 + vol_range[cn].wNumSubRanges * sizeof(layout2_range_t));
		}
	}
	USB_TODO();
	return -1;
}

static uint32_t ae_fu_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	USB_TODO();
	return -1;
}

// Input terminal
static uint8_t ae_it_usb_desc(void *p, uint32_t size)
{
	const desc_it_t desc = {
		.bLength = 17,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = INPUT_TERMINAL,
		.bTerminalID = USB_AUDIO2_ENTITY_ID(ae_it_usb),
		.wTerminalType = USB_streaming,
		.bAssocTerminal = 0,
		.bCSourceID = USB_AUDIO2_ENTITY_ID(ae_cs),
		{.bNrChannels = 2,
		.bmChannelConfig = SP_FL | SP_FR,	// Front left, front right
		.iChannelNames = USB_DESC_STRING_INDEX(ae_it_usb_str)},
		.bmControls = 0,
		.iTerminal = USB_DESC_STRING_INDEX(ae_it_usb_str),
	};
#if DEBUG >= 5
	if (desc.bLength > size)
		USB_ERROR();
#endif
	*(desc_it_t *)p = desc;
	return desc.bLength;
}

static uint32_t ae_it_usb_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	USB_TODO();
	return -1;
}

static uint32_t ae_it_usb_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	USB_TODO();
	return -1;
}

// Output Terminal
static uint8_t ae_ot_hp_desc(void *p, uint32_t size)
{
	const desc_ot_t desc = {
		.bLength = 12,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = OUTPUT_TERMINAL,
		.bTerminalID = USB_AUDIO2_ENTITY_ID(ae_ot_hp),
		.wTerminalType = Headphones,
		.bAssocTerminal = 0,
		.bSourceID = USB_AUDIO2_ENTITY_ID(ae_fu),
		.bCSourceID = USB_AUDIO2_ENTITY_ID(ae_cs),
		.bmControls = 0,
		.iTerminal = USB_DESC_STRING_INDEX(ae_ot_hp_str),
	};
#if DEBUG >= 5
	if (desc.bLength > size)
		USB_ERROR();
#endif
	*(desc_ot_t *)p = desc;
	return desc.bLength;
}

static uint32_t ae_ot_hp_get(void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	USB_TODO();
	return -1;
}

static uint32_t ae_ot_hp_set(const void *p, uint32_t size, uint8_t request, uint8_t cs, uint8_t cn, uint16_t length)
{
	USB_TODO();
	return -1;
}

// Audio Streaming Interface
USB_DESC_STRING(as_in_str) = {0, LANG_EN_US, "Audio Streaming - IN"};

static uint8_t as_in_terminal(const usb_audio2_streaming_t *pas)
{
	return USB_AUDIO2_ENTITY_ID(ae_it_usb);
}

static uint8_t as_in_desc(uint8_t mode, void *p, uint32_t size)
{
	static const desc_type_i_t descfmt[2] = {{
		.bLength = 6,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = FORMAT_TYPE,
		.bFormatType = FORMAT_TYPE_I,
		.bSubslotSize = 4,
		.bBitResolution = 32,
	}, {
		.bLength = 6,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = FORMAT_TYPE,
		.bFormatType = FORMAT_TYPE_I,
		.bSubslotSize = 2,
		.bBitResolution = 16,
	}};

	if (mode == 0 || mode > ASIZE(descfmt))
		return 0;

	const desc_as_t desc = {
		.bLength = 16,
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = AS_GENERAL,
		.bTerminalLink = USB_AUDIO2_ENTITY_ID(ae_it_usb),
		.bmControls = 0,
		.bFormatType = FORMAT_TYPE_I,
		.bmFormats = FRMT_I_PCM,
		{.bNrChannels = 2,
		.bmChannelConfig = SP_FL | SP_FR,
		.iChannelNames = USB_DESC_STRING_INDEX(as_in_str)}
	};

	uint32_t i = mode - 1;
#if DEBUG >= 5
	if (desc.bLength > size)
		USB_ERROR();
#endif
	uint8_t bLength = desc.bLength;
	*(desc_as_t *)p = desc;
	p += bLength;
	size -= bLength;

#if DEBUG >= 5
	if (descfmt[i].bLength > size)
		USB_ERROR();
#endif
	bLength += descfmt[i].bLength;
	*(desc_type_i_t *)p = descfmt[i];
	p += bLength;
	size -= bLength;
	return bLength;
}

static uint32_t as_in_mode(uint8_t mode)
{
	return audio_set_mode(mode);
}

static usb_audio2_data_t as_in_data;
USB_AUDIO2_STREAMING() = {
	.type = UsbAudio2In,
	.data = &as_in_data,
	.terminal = &as_in_terminal,
	.desc = &as_in_desc,
	.set_mode = &as_in_mode,
};

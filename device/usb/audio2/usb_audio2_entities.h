#ifndef USB_AUDIO2_ENTITIES_H
#define USB_AUDIO2_ENTITIES_H

#include <debug.h>
#include "../usb_setup.h"

typedef struct usb_audio_t usb_audio_t;
typedef struct usb_audio_entity_t usb_audio_entity_t;

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

// Entity private data structures

typedef struct {
	// CS_CLOCK_VALID_CONTROL: Parameter layout 1
	layout1_cur_t (*valid)(usb_audio_t *audio, const uint8_t id);
	// CS_SAM_FREQ_CONTROL: Parameter layout 3
	layout3_cur_t (*sam_freq)(usb_audio_t *audio, const uint8_t id);
	int (*sam_freq_set)(usb_audio_t *audio, const uint8_t id,
			    const layout3_cur_t v);
	uint32_t (*sam_freq_range)(usb_audio_t *audio, const uint8_t id,
				   layout3_range_t *buf);
} audio_cs_t;

typedef struct {
	// CX_CLOCK_SELECTOR_CONTROL: Parameter layout 1
	layout1_cur_t (*selector)(usb_audio_t *audio, const uint8_t id);
} audio_cx_t;

typedef struct {
	int channels;
	// FU_MUTE_CONTROL: Parameter layout 1
	layout1_cur_t (*mute)(usb_audio_t *audio, const uint8_t id,
			      const int cn);
	int (*mute_set)(usb_audio_t *audio, const uint8_t id,
			const int cn, layout1_cur_t v);
	// FU_VOLUME_CONTROL: Parameter layout 2
	layout2_cur_t (*volume)(usb_audio_t *audio, const uint8_t id,
				const int cn);
	int (*volume_set)(usb_audio_t *audio, const uint8_t id,
			  const int cn, const layout2_cur_t v);
	uint32_t (*volume_range)(usb_audio_t *audio, const uint8_t id,
				 const int cn, layout2_range_t *buf);
} audio_fu_t;

// Parameter block operations

static inline uint32_t layout_cur(int type, void *p)
{
	uint32_t v = 0;
	const unsigned int msize = 1u << (type - 1u);
	memcpy(&v, p, msize);
	dbgprintf("%lx)", v);
	return v;
}

// Initialisation and register functions
void usb_audio2_entities_init(usb_audio_t *data);
void usb_audio2_register(usb_audio_t *data, void (*config)(usb_t *, usb_audio_t *));
usb_audio_entity_t *usb_audio2_register_entity(usb_audio_t *audio, const uint8_t id, const void *data);
// Clock source
void usb_audio2_register_cs(usb_audio_t *audio, const uint8_t id, const audio_cs_t *data,
			    usb_t *usb, uint8_t bmAttributes, uint8_t bmControls,
			    uint8_t bAssocTerminal, uint8_t iClockSource);
// Clock selector
void usb_audio2_register_cx(usb_audio_t *audio, const uint8_t id, const audio_cs_t *data,
			    usb_t *usb, uint8_t bNrInPins, uint8_t *baCSourceID,
			    uint8_t bmControls, uint8_t iClockSelector);
// Feature unit
void usb_audio2_register_fu(usb_audio_t *audio, const uint8_t id, const audio_fu_t *data,
			    usb_t *usb, uint8_t bSourceID,
			    uint32_t *bmaControls, uint8_t iFeature);

// USB protocol
void usb_audio2_get(usb_t *usb, usb_audio_t *data, uint32_t ep, setup_t pkt);
void usb_audio2_set(usb_t *usb, usb_audio_t *data, uint32_t ep, setup_t pkt);

#endif // USB_AUDIO2_ENTITIES_H

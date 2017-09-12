#ifndef USB_AUDIO2_STRUCTS_H
#define USB_AUDIO2_STRUCTS_H

#include <string.h>
#include <debug.h>
#include <macros.h>
#include <peripheral/audio.h>
#include "../usb_setup.h"
#include "../usb_desc.h"
#include "usb_audio2_entities.h"

#define DATA_MAX_SIZE	(256u)
#define CHANNELS	(1u + AUDIO_CHANNELS)

#ifdef __cplusplus
extern "C" {
#endif

// Private data types

typedef struct usb_audio_entity_t {
	struct usb_audio_entity_t *next;
	desc_t (*get)(usb_audio_t *data, struct usb_audio_entity_t *entity, setup_t pkt);
	int (*set)(usb_audio_t *data, struct usb_audio_entity_t *entity, setup_t pkt);
	uint8_t id;		// Entity ID
	const void *data;	// Entity private data
	struct PACKED {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t raw[0];
	} *desc;		// Entity USB descriptor
} usb_audio_entity_t;

typedef struct usb_audio_t {
	usb_audio_entity_t *entities;
	struct PACKED {
		union {
			uint8_t raw[48];
			struct {
				uint16_t wNumSubRanges;
				union {
					uint8_t range[0];
					layout1_range_t range1[0];
					layout2_range_t range2[0];
					layout3_range_t range3[0];
				};
			};
			layout1_cur_t cur1;
			layout2_cur_t cur2;
			layout3_cur_t cur3;
		};
	} buf;
	int ep_data, ep_feedback;
} usb_audio_t;

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO2_STRUCTS_H

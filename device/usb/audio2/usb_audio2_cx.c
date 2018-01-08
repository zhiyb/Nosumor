#include <malloc.h>
#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"
#include "usb_audio2_desc.h"

desc_t usb_audio2_cx_get(usb_audio_t *audio, usb_audio_entity_t *entity, setup_t pkt)
{
	const audio_cx_t *data = entity->data;
	desc_t desc = {0, audio->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_READ "(CX_");
	switch (cs) {
	case CX_CLOCK_SELECTOR_CONTROL:
		// Layout 1 parameter block
		if (cn != 0) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("c");
			audio->buf.cur1 = data->selector(audio, entity->id);
			desc.size = sizeof(layout1_cur_t);
			break;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	dbgprintf("%u/%lu@%u)", cn, desc.size, pkt.wLength);
	return desc;
}

void usb_audio2_register_cx(usb_audio_t *audio, const uint8_t id, const audio_cx_t *data,
			    uint8_t bNrInPins, const uint8_t *baCSourceID,
			    uint8_t bmControls, uint8_t iClockSelector)
{
	// Register entitry
	usb_audio_entity_t *entity = usb_audio2_register_entity(audio, id, data);
	entity->get = &usb_audio2_cx_get;
	entity->set = 0;
	// Add descriptor
	uint8_t bLength = 7u + bNrInPins;
	desc_cx_t *desc = malloc(bLength);
	desc->bLength = bLength;
	desc->bDescriptorType = CS_INTERFACE;
	desc->bDescriptorSubtype = CLOCK_SELECTOR;
	desc->bClockID = id;
	desc->bNrInPins = bNrInPins;
	memcpy(desc->baCSourceID, baCSourceID, bNrInPins);
	*((uint8_t *)desc + bLength - 2u) = bmControls;
	*((uint8_t *)desc + bLength - 1u) = iClockSelector;
	entity->desc = (void *)desc;
}

#include <malloc.h>
#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"
#include "usb_audio2_desc.h"

desc_t usb_audio2_cs_get(usb_audio_t *audio, usb_audio_entity_t *entity, setup_t pkt)
{
	const audio_cs_t *data = entity->data;
	desc_t desc = {0, audio->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_GREEN "(CS_");
	switch (cs) {
	case CS_SAM_FREQ_CONTROL:
		// Layout 3 parameter block
		if (cn != 0) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("f");
			audio->buf.cur3 = data->sam_freq(audio, entity->id);
			desc.size = sizeof(layout3_cur_t);
			break;
		case RANGE:
			dbgprintf("fr");
			audio->buf.wNumSubRanges = data->sam_freq_range(audio, entity->id, audio->buf.range3);
			desc.size = 2u + audio->buf.wNumSubRanges * sizeof(layout3_range_t);
			break;
		default:
			dbgbkpt();
		}
		break;
	case CS_CLOCK_VALID_CONTROL:
		// Layout 1 parameter block
		if (cn != 0) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("v");
			audio->buf.cur1 = data->valid(audio, entity->id);
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

int usb_audio2_cs_set(usb_audio_t *audio, usb_audio_entity_t *entity, setup_t pkt)
{
	const audio_cs_t *data = entity->data;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_RED "(CS_");
	switch (cs) {
	case CS_SAM_FREQ_CONTROL:
		// Layout 3 parameter block
		if (cn != 0) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("F");
			// TODO: Check frequency validity
			return data->sam_freq_set(audio, entity->id, layout_cur(3, pkt.data));
		default:
			dbgbkpt();
		}
	default:
		dbgbkpt();
	}
	return 0;
}

void usb_audio2_register_cs(usb_audio_t *audio, const uint8_t id, const audio_cs_t *data,
			    usb_t *usb, uint8_t bmAttributes, uint8_t bmControls,
			    uint8_t bAssocTerminal, uint8_t iClockSource)
{
	// Register entitry
	usb_audio_entity_t *entity = usb_audio2_register_entity(audio, id, data);
	entity->get = &usb_audio2_cs_get;
	entity->set = &usb_audio2_cs_set;
	// Add descriptor
	const desc_cs_t desc = {
		8u, CS_INTERFACE, CLOCK_SOURCE, id,
		bmAttributes, bmControls, bAssocTerminal, iClockSource
	};
	void *pd = malloc(desc.bLength);
	if (!pd)
		panic();
	memcpy(pd, &desc, desc.bLength);
	entity->desc = pd;
}

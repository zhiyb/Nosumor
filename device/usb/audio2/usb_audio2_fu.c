#include <malloc.h>
#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"
#include "usb_audio2_desc.h"

desc_t usb_audio2_fu_get(usb_audio_t *audio, usb_audio_entity_t *entity, setup_t pkt)
{
	const audio_fu_t *data = entity->data;
	desc_t desc = {0, audio->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_GREEN "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn > data->channels) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("m");
			audio->buf.cur1 = data->mute(audio, entity->id, cn);
			desc.size = sizeof(layout1_cur_t);
			break;
		default:
			dbgbkpt();
		}
		break;
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn > data->channels) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("v");
			audio->buf.cur2 = data->volume(audio, entity->id, cn);
			desc.size = sizeof(layout2_cur_t);
			break;
		case RANGE:
			dbgprintf("vr");
			audio->buf.wNumSubRanges = data->volume_range(audio, entity->id, cn, audio->buf.range2);
			desc.size = 2u + audio->buf.wNumSubRanges * sizeof(layout2_range_t);
			break;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	dbgprintf("%u|%lu@%u)", cn, desc.size, pkt.wLength);
	return desc;
}

int usb_audio2_fu_set(usb_audio_t *audio, usb_audio_entity_t *entity, setup_t pkt)
{
	const audio_fu_t *data = entity->data;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_RED "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn > data->channels) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("M%u|", cn);
			return data->mute_set(audio, entity->id, cn, layout_cur(1, pkt.data));
		default:
			dbgbkpt();
		}
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn > data->channels) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR: {
			dbgprintf("V%u|", cn);
			return data->volume_set(audio, entity->id, cn, layout_cur(2, pkt.data));
		}
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	return 0;
}

void usb_audio2_register_fu(usb_audio_t *audio, const uint8_t id, const audio_fu_t *data,
			    uint8_t bSourceID, const uint32_t *bmaControls, uint8_t iFeature)
{
	// Register entitry
	usb_audio_entity_t *entity = usb_audio2_register_entity(audio, id, data);
	entity->get = &usb_audio2_fu_get;
	entity->set = &usb_audio2_fu_set;
	// Add descriptor
	uint8_t bmaLength = (data->channels + 1u) * 4u;
	uint8_t bLength = 6u + bmaLength;
	desc_fu_t *pd = malloc(bLength);
	pd->bLength = bLength;
	pd->bDescriptorType = CS_INTERFACE;
	pd->bDescriptorSubtype = FEATURE_UNIT;
	pd->bUnitID = id;
	pd->bSourceID = bSourceID;
	memcpy(pd->bmaControls, bmaControls, bmaLength);
	*((uint8_t *)pd + bLength - 1u) = iFeature;
	entity->desc = (void *)pd;
}

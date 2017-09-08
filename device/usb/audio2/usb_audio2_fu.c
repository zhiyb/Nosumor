#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"

desc_t fu_get(data_t *data, setup_t pkt)
{
	desc_t desc = {0, data->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_GREEN "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn >= ASIZE(data->fu.mute)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("m");
			desc.size = layout_cur_get(data, 1, &data->fu.mute[cn]);
			break;
		default:
			dbgbkpt();
		}
		break;
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn >= ASIZE(data->fu.vol)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("v");
			desc.size = layout_cur_get(data, 2, &data->fu.vol[cn]);
			break;
		case RANGE:
			dbgprintf("vr");
			desc.size = layout_range_get(data, 2, &data->fu.range[cn], 1u);
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

int fu_set(data_t *data, setup_t pkt, void *buf)
{
	uint16_t v;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_RED "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn >= ASIZE(data->fu.mute)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("M");
			data->fu.mute[cn] = layout_cur(1, buf);
			dbgprintf("%u)", cn);
			return 1;
		default:
			dbgbkpt();
		}
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn >= ASIZE(data->fu.vol)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("V");
			data->fu.vol[cn] = layout_cur(2, buf);
			dbgprintf("%u)", cn);
			return 1;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	return 0;
}

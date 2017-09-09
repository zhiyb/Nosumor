#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"

void usb_audio2_cs_init(data_t *data)
{
	data->cs.freq = 192000;
	data->cs.range[0].min = 192000;
	data->cs.range[0].max = 192000;
	data->cs.range[0].res = 0;
	data->cs.valid = 1;
}

desc_t usb_audio2_cs_get(data_t *data, setup_t pkt)
{
	desc_t desc = {0, data->buf.raw};
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
			desc.size = layout_cur_get(data, 3, &data->cs.freq);
			break;
		case RANGE:
			dbgprintf("fr");
			desc.size = layout_range_get(data, 3, &data->cs.range[0], ASIZE(data->cs.range));
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
			desc.size = layout_cur_get(data, 1, &data->cs.valid);
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

int usb_audio2_cs_set(data_t *data, setup_t pkt, void *buf)
{
	uint16_t v;
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
			data->cs.freq = layout_cur(3, buf);
			dbgprintf("%lu)", data->cs.freq);
			return 1;
		default:
			dbgbkpt();
		}
	default:
		dbgbkpt();
	}
	return 0;
}

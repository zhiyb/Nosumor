#include <peripheral/audio.h>
#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"

void usb_audio2_fu_init(data_t *data)
{
	data->fu.mute[0] = 0x00;
	data->fu.vol[0] = audio_sp_vol_min();
	data->fu.range[0].min = audio_sp_vol_min();
	data->fu.range[0].max = audio_sp_vol_max();
	data->fu.range[0].res = audio_sp_vol_res();
	for (int i = 1; i != CHANNELS; i++) {
		data->fu.mute[i] = 0x00;
		data->fu.vol[i] = audio_ch_vol_min();
		data->fu.range[i].min = audio_ch_vol_min();
		data->fu.range[i].max = audio_ch_vol_max();
		data->fu.range[i].res = audio_ch_vol_res();
	}
}

desc_t usb_audio2_fu_get(data_t *data, setup_t pkt)
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

int usb_audio2_fu_set(data_t *data, setup_t pkt, void *buf)
{
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
		case CUR: {
			dbgprintf("V");
			int16_t v = layout_cur(2, buf);
			data->fu.vol[cn] = v;
			if (cn == 0) {
				audio_sp_vol_set_async(0u, v);
				audio_sp_vol_set_async(1u, v);
			} else {
				audio_ch_vol_set_async(cn - 1u, v);
			}
			dbgprintf("%u)", cn);
			return 1;
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

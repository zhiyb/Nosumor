#include "../usb_desc.h"
#include "usb_audio2_defs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"

desc_t cs_get(data_t *data, setup_t pkt)
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
			desc.size = layout_cur_get(data, 3, &data->clk.freq[0]);
			break;
		case RANGE:
			dbgprintf("fr");
			desc.size = layout_range_get(data, 3, &data->clk.range[0], 1u);
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

#include "../usb_desc.h"
#include "../usb_setup.h"
#include "../usb_ep.h"
#include "../usb_structs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_desc.h"
#include "usb_audio2_entities.h"

/* External functions */

// Clock source
desc_t usb_audio2_cs_get(data_t *data, setup_t pkt);
int usb_audio2_cs_set(data_t *data, setup_t pkt, void *buf);

// Clock selector
desc_t usb_audio2_cx_get(data_t *data, setup_t pkt);

// Feature unit
desc_t usb_audio2_fu_get(data_t *data, setup_t pkt);
int usb_audio2_fu_set(data_t *data, setup_t pkt, void *buf);

/* USB packet decodeing */

void usb_audio2_get(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	desc_t desc = {0, 0};
	desc_t (*func)(data_t *data, setup_t pkt) = 0;
	switch (pkt.bEntityID) {
	case CS_PLL:
		func = usb_audio2_cs_get;
		break;
	case CX_IN:
		func = usb_audio2_cx_get;
		break;
	case FU_Out:
		func = usb_audio2_fu_get;
		break;
	}
	if (func)
		desc = func(data, pkt);
	if (desc.size == 0) {
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	} else {
		if (desc.size > pkt.wLength)
			desc.size = pkt.wLength;
		usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
	}
}

void usb_audio2_set(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	int (*func)(data_t *data, setup_t pkt, void *buf) = 0;
	switch (pkt.bEntityID) {
	case CS_PLL:
		func = usb_audio2_cs_set;
		break;
	case FU_Out:
		func = usb_audio2_fu_set;
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
	if (func) {
		if (func(data, pkt, usb->setup_buf))
			usb_ep_in_transfer(usb->base, ep, 0, 0);
		else
			usb_ep_in_stall(usb->base, ep);
	}
}

void usb_audio2_entities_init(data_t *data)
{
	data->cs.freq = 192000;
	data->cs.range[0].min = 192000;
	data->cs.range[0].max = 192000;
	data->cs.range[0].res = 0;
	for (int i = 0; i != CHANNELS; i++) {
		data->fu.mute[i] = 0x00;
		data->fu.vol[i] = 0x0000;
		data->fu.range[i].min = audio_vol_min();
		data->fu.range[i].max = audio_vol_max();
		data->fu.range[i].res = audio_vol_res();
	}
}

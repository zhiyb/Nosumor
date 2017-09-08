#include "../usb_desc.h"
#include "../usb_setup.h"
#include "../usb_ep.h"
#include "../usb_structs.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_desc.h"
#include "usb_audio2_entities.h"

/* External functions */

// Clock source
desc_t cs_get(data_t *data, setup_t pkt);

// Clock selector
desc_t cx_get(data_t *data, setup_t pkt);

// Feature unit
desc_t fu_get(data_t *data, setup_t pkt);
int fu_set(data_t *data, setup_t pkt, void *buf);

/* USB packet decodeing */

void usb_get(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	desc_t desc = {0, 0};
	switch (pkt.bEntityID) {
	case CS_USB:
		desc = cs_get(data, pkt);
		break;
	case CX_IN:
		desc = cx_get(data, pkt);
		break;
	case FU_Out:
		desc = fu_get(data, pkt);
		break;
	}
	if (desc.size == 0) {
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	} else {
		if (desc.size > pkt.wLength)
			desc.size = pkt.wLength;
		usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
	}
}

void usb_set(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bEntityID) {
	case FU_Out:
		if (fu_set(data, pkt, usb->setup_buf))
			usb_ep_in_transfer(usb->base, ep, 0, 0);
		else
			usb_ep_in_stall(usb->base, ep);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

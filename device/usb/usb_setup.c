#include <debug.h>
#include "usb_setup.h"
#include "usb_ep.h"
#include "usb_macros.h"
#include "usb_desc.h"
#include "usb_ram.h"

static void usb_send_descriptor(usb_t *usb, uint32_t ep, setup_t pkt)
{
	desc_t desc;
	switch (pkt.dtype) {
	case SETUP_DESC_TYPE_DEVICE:
		desc = usb_desc_device(usb);
		break;
	case SETUP_DESC_TYPE_CONFIGURATION:
		desc = usb_desc_config(usb);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		return;
	}
	desc.size = desc.size > pkt.length ? pkt.length : desc.size;
	usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
}

static void usb_setup_standard_device(usb_t *usb, uint32_t ep, setup_t pkt)
{
	switch (pkt.type & SETUP_TYPE_DIR_Msk) {
	case SETUP_TYPE_DIR_D2H:
		switch (pkt.request) {
		case SETUP_REQ_GET_DESCRIPTOR:
			usb_send_descriptor(usb, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.request) {
		case SETUP_REQ_SET_ADDRESS:
			DEVICE(usb->base)->DCFG = (DEVICE(usb->base)->DCFG & ~USB_OTG_DCFG_DAD_Msk) |
					((pkt.value << USB_OTG_DCFG_DAD_Pos) & USB_OTG_DCFG_DAD_Msk);
			usb_ep_in_transfer(usb->base, ep, 0, 0);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

void usb_setup(usb_t *usb, uint32_t stat)
{
	uint32_t ep = STAT_EP(stat);
	uint32_t cnt = STAT_CNT(stat);
	if (cnt != 8)
		dbgbkpt();
	cnt = (cnt + 3) >> 2;

	// Receive setup packet from FIFO
	setup_t pkt;
	uint32_t *p = (uint32_t *)&pkt;
	while (cnt--)
		*p++ = FIFO(usb->base, ep);

	// Process setup packet
	switch (pkt.type & SETUP_TYPE_TYPE_Msk) {
	case SETUP_TYPE_TYPE_STD:
		switch (pkt.type & SETUP_TYPE_RCPT_Msk) {
		case SETUP_TYPE_RCPT_DEV:
			usb_setup_standard_device(usb, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_TYPE_CLASS:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		break;
	}

	// Reset endpoint setup packet counter
	EP_OUT(usb->base, ep)->DOEPTSIZ |= USB_OTG_DOEPTSIZ_STUPCNT_Msk | USB_OTG_DOEPTSIZ_PKTCNT_Msk;
	// Enable endpoint
	EP_OUT(usb->base, ep)->DOEPCTL |= USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk;
}

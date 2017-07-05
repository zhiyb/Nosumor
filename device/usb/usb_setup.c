#include <debug.h>
#include "usb_setup.h"
#include "usb_ep.h"
#include "usb_macros.h"
#include "usb_desc.h"
#include "usb_ram.h"
#include "usb.h"

static void usb_send_descriptor(usb_t *usb, uint32_t ep, setup_t pkt)
{
	desc_t desc;
	switch (pkt.bType) {
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
	desc.size = desc.size > pkt.wLength ? pkt.wLength : desc.size;
	usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
}

static void usb_setup_standard_device(usb_t *usb, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_Msk) {
	case SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case SETUP_REQ_GET_DESCRIPTOR:
			usb_send_descriptor(usb, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_ADDRESS:
			usb_ep_in_transfer(usb->base, ep, 0, 0);
			DEVICE(usb->base)->DCFG = (DEVICE(usb->base)->DCFG & ~USB_OTG_DCFG_DAD_Msk) |
					((pkt.wValue << USB_OTG_DCFG_DAD_Pos) & USB_OTG_DCFG_DAD_Msk);
			break;
		case SETUP_REQ_SET_CONFIGURATION:
			if (pkt.wValue == 1) {
				for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
					FUNC((*ip)->enable)(usb, (*ip)->data);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
			} else {
				usb_ep_in_stall(usb->base, ep);
				dbgbkpt();
			}
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

static void usb_setup_standard_interface(usb_t *usb, uint32_t ep, setup_t pkt)
{
	uint32_t i = pkt.wIndex;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
		if ((*ip)->id == i && (*ip)->setup_std) {
			(*ip)->setup_std(usb, (*ip)->data, ep, pkt);
			return;
		}
	usb_ep_in_stall(usb->base, ep);
	dbgbkpt();
}

static void usb_setup_standard_endpoint(usb_t *usb, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_Msk) {
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_CLEAR_FEATURE:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

static void usb_setup_class_interface(usb_t *usb, uint32_t ep, setup_t pkt)
{
	uint32_t i = pkt.wIndex;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
		if ((*ip)->id == i && (*ip)->setup_class) {
			(*ip)->setup_class(usb, (*ip)->data, ep, pkt);
			return;
		}
	usb_ep_in_stall(usb->base, ep);
	dbgbkpt();
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
	switch (pkt.bmRequestType & SETUP_TYPE_TYPE_Msk) {
	case SETUP_TYPE_TYPE_STD:
		switch (pkt.bmRequestType & SETUP_TYPE_RCPT_Msk) {
		case SETUP_TYPE_RCPT_DEVICE:
			usb_setup_standard_device(usb, ep, pkt);
			break;
		case SETUP_TYPE_RCPT_INTERFACE:
			usb_setup_standard_interface(usb, ep, pkt);
			break;
		case SETUP_TYPE_RCPT_ENDPOINT:
			usb_setup_standard_endpoint(usb, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_TYPE_CLASS:
		switch (pkt.bmRequestType & SETUP_TYPE_RCPT_Msk) {
		case SETUP_TYPE_RCPT_INTERFACE:
			usb_setup_class_interface(usb, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
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

#include "../debug.h"
#include "usb_setup.h"
#include "usb_ep.h"
#include "usb_macros.h"
#include "usb_structs.h"

#define TYPE_Pos	5u
#define TYPE_Msk	(3ul << TYPE_Pos)
#define TYPE_STD	(0ul << TYPE_Pos)
#define TYPE_CLASS	(1ul << TYPE_Pos)
#define TYPE_VENDOR	(2ul << TYPE_Pos)

#define RCPT_Pos	0u
#define RCPT_Msk	(0x1ful << RCPT_Pos)
#define RCPT_DEVICE	(0ul << RCPT_Pos)
#define RCPT_INTERFACE	(1ul << RCPT_Pos)
#define RCPT_ENDPOINT	(2ul << RCPT_Pos)
#define RCPT_OTHER	(3ul << RCPT_Pos)

static void usb_setup_standard_device(usb_t *usb, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case GET_DESCRIPTOR: {
			desc_t desc = usb_get_descriptor(usb, pkt);
			if (desc.size != 0) {
				desc.size = desc.size > pkt.wLength ? pkt.wLength : desc.size;
				usb_ep_in_descriptor(usb->base, ep, desc);
			} else {
				dbgprintf(ESC_MAGENTA "<D%x>", pkt.bType);
				usb_ep_in_stall(usb->base, ep);
			}
			break;
		}
		case GET_STATUS:
			if (pkt.wValue != 0 || pkt.wIndex != 0 || pkt.wLength != 2) {
				usb_ep_in_stall(usb->base, ep);
				dbgbkpt();
				break;
			}
			usb_ep_in_transfer(usb->base, ep, &usb->status, pkt.wLength);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
		switch (pkt.bRequest) {
		case SET_ADDRESS:
			//usb->addr = pkt.wValue;
			DEV(usb->base)->DCFG = (DEV(usb->base)->DCFG & ~USB_OTG_DCFG_DAD_Msk) |
					((pkt.wValue << USB_OTG_DCFG_DAD_Pos) & USB_OTG_DCFG_DAD_Msk);
			usb_ep_in_transfer(usb->base, ep, 0, 0);
			break;
		case SET_CONFIGURATION:
			if (pkt.wValue == 1) {
				// Initialise endpoints
				for (int i = 1; i != USB_EPIN_CNT; i++)
					FUNC(usb->epin[i].init)(usb, i);
				for (int i = 1; i != USB_EPIN_CNT; i++)
					FUNC(usb->epout[i].init)(usb, i);
				// Enable endpoints
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
	uint32_t i = pkt.bID;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
		if ((*ip)->id == i && (*ip)->setup_std) {
			(*ip)->setup_std(usb, (*ip)->data, ep, pkt);
			return;
		}
	usb_ep_in_stall(usb->base, ep);
	dbgbkpt();
}

static int usb_setup_endpoint_halt(usb_t *usb, uint32_t ep, int halt)
{
	uint32_t n = ep & ~EP_DIR_Msk;
	if (n == 0)
		return 1;
	switch (ep & EP_DIR_Msk) {
	case EP_DIR_IN:
		if (usb->epin[n].halt) {
			usb->epin[n].halt(usb, n, halt);
			return 1;
		}
		dbgbkpt();
		break;
	case EP_DIR_OUT:
		if (usb->epout[n].halt) {
			usb->epout[n].halt(usb, n, halt);
			return 1;
		}
		dbgbkpt();
		break;
	default:
		dbgbkpt();
	}
	return 0;

}

static void usb_setup_standard_endpoint(usb_t *usb, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_H2D:
		switch (pkt.bRequest) {
		case CLEAR_FEATURE:
			switch (pkt.wValue) {
			case FEATURE_ENDPOINT_HALT:
				if (usb_setup_endpoint_halt(usb, pkt.wIndex, 0))
					usb_ep_in_transfer(usb->base, ep, 0, 0);
				else
					usb_ep_in_stall(usb->base, ep);
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
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

static void usb_setup_class_interface(usb_t *usb, uint32_t ep, setup_t pkt)
{
	uint32_t i = pkt.bID;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
		if ((*ip)->id == i && (*ip)->setup_class) {
			(*ip)->setup_class(usb, (*ip)->data, ep, pkt);
			return;
		}
	usb_ep_in_stall(usb->base, ep);
	dbgbkpt();
}

void usb_setup(usb_t *usb, uint32_t n, setup_t pkt)
{
	// Process setup packet
	switch (pkt.bmRequestType & TYPE_Msk) {
	case TYPE_STD:
		switch (pkt.bmRequestType & RCPT_Msk) {
		case RCPT_DEVICE:
			usb_setup_standard_device(usb, n, pkt);
			break;
		case RCPT_INTERFACE:
			usb_setup_standard_interface(usb, n, pkt);
			break;
		case RCPT_ENDPOINT:
			usb_setup_standard_endpoint(usb, n, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, n);
			dbgbkpt();
		}
		break;
	case TYPE_CLASS:
		switch (pkt.bmRequestType & RCPT_Msk) {
		case RCPT_INTERFACE:
			usb_setup_class_interface(usb, n, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, n);
			dbgbkpt();
		}
		break;
	default:
		usb_ep_in_stall(usb->base, n);
		dbgbkpt();
		break;
	}
}

#include <debug.h>
#include <usb/usb.h>
#include <usb/usb_hw.h>
#include <usb/usb_macros.h>
#include <usb/usb_desc.h>
#include <usb/usb_hw.h>
#include <usb/usb_hw_macros.h>
#include "usb_ep0_setup.h"
#include "usb_ep0.h"

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

#define DIR_Pos		7u
#define DIR_Msk		(1ul << DIR_Pos)
#define DIR_H2D		(0ul << DIR_Pos)
#define DIR_D2H		(1ul << DIR_Pos)

#define GET_STATUS		0u
#define CLEAR_FEATURE		1u
#define SET_FEATURE		3u
#define SET_ADDRESS		5u
#define GET_DESCRIPTOR		6u
#define SET_DESCRIPTOR		7u
#define GET_CONFIGURATION	8u
#define SET_CONFIGURATION	9u
#define GET_INTERFACE		10u
#define SET_INTERFACE		11u
#define SYNCH_FRAME		12u

#define FEATURE_ENDPOINT_HALT		0u
#define FEATURE_DEVICE_REMOTE_WAKEUP	1u
#define FEATURE_TEST_MODE		2u

static void usb_setup_standard_device(usb_setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case GET_DESCRIPTOR: {
			usb_desc_t desc;
			desc.p = usb_ep0_in_buf(&desc.size);
			usb_get_descriptor(pkt, &desc);
			if (desc.size != 0)
				usb_ep0_in(desc.size > pkt.wLength ? pkt.wLength : desc.size);
			else
				USB_ERROR();
			break;
		}
		case GET_STATUS:
			if (pkt.wValue != 0 || pkt.wIndex != 0 || pkt.wLength != 2) {
				//usb_ep_in_stall(usb->base, ep);
				USB_TODO();
				break;
			}
			USB_TODO();
			//usb_ep_in_transfer(usb->base, ep, &usb->status, pkt.wLength);
			break;
		default:
			//usb_ep_in_stall(usb->base, ep);
			USB_TODO();
		}
		break;
	case DIR_H2D:
		switch (pkt.bRequest) {
		case SET_ADDRESS:
			usb_hw_set_addr(pkt.wValue);
			usb_ep0_in_buf(0);
			usb_ep0_in(0);
			break;
		case SET_CONFIGURATION:
			if (pkt.wValue == 1) {
				// Initialise endpoints
				USB_TODO();
				/*for (int i = 1; i != USB_EPIN_CNT; i++)
					FUNC(usb->epin[i].init)(usb, i);
				for (int i = 1; i != USB_EPOUT_CNT; i++)
					FUNC(usb->epout[i].init)(usb, i);
				// Enable endpoints
				for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next)
					FUNC((*ip)->enable)(usb, (*ip)->data);
				usb_ep_in_transfer(usb->base, ep, 0, 0);*/
			} else {
				//usb_ep_in_stall(usb->base, ep);
				USB_TODO();
			}
			break;
		default:
			//usb_ep_in_stall(usb->base, ep);
			USB_TODO();
		}
		break;
	default:
		//usb_ep_in_stall(usb->base, ep);
		USB_TODO();
	}
}

void usb_ep0_setup(void *p, uint32_t size)
{
	usb_setup_t pkt = *(usb_setup_t *)p;
	if (size != 8)
		USB_TODO();

	// Process setup packet
	switch (pkt.bmRequestType & TYPE_Msk) {
	case TYPE_STD:
		switch (pkt.bmRequestType & RCPT_Msk) {
		case RCPT_DEVICE:
			usb_setup_standard_device(pkt);
			break;
		case RCPT_INTERFACE:
			USB_TODO();
			//usb_setup_standard_interface(usb, n, pkt);
			break;
		case RCPT_ENDPOINT:
			USB_TODO();
			//usb_setup_standard_endpoint(usb, n, pkt);
			break;
		default:
			//usb_ep_in_stall(usb->base, n);
			USB_TODO();
		}
		break;
	case TYPE_CLASS:
		switch (pkt.bmRequestType & RCPT_Msk) {
		case RCPT_INTERFACE:
			USB_TODO();
			//usb_setup_class_interface(usb, n, pkt);
			break;
		default:
			//usb_ep_in_stall(usb->base, n);
			USB_TODO();
		}
		break;
	default:
		//usb_ep_in_stall(usb->base, n);
		USB_TODO();
		break;
	}
}

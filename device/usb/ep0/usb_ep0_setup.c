#include <debug.h>
#include <usb/usb.h>
#include <usb/usb_hw.h>
#include <usb/usb_macros.h>
#include <usb/usb_desc.h>
#include <usb/usb_hw.h>
#include <usb/usb_hw_macros.h>
#include "usb_ep0_setup.h"
#include "usb_ep0.h"

#define USB_MAX_IFACE	8

#define RCPT_Pos	0u
#define RCPT_Msk	(0x1ful << RCPT_Pos)
#define RCPT_DEVICE	(0ul << RCPT_Pos)
#define RCPT_INTERFACE	(1ul << RCPT_Pos)
#define RCPT_ENDPOINT	(2ul << RCPT_Pos)
#define RCPT_OTHER	(3ul << RCPT_Pos)

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

LIST(usb_config, usb_config_handler_t);

static struct {
	struct {
		usb_ep0_iface_setup_t setup;
	} iface[USB_MAX_IFACE];
} data;

void usb_setup_data_reset()
{
	for (int i = 0; i < USB_MAX_IFACE; i++)
		data.iface[i].setup = 0;
}

USB_RESET_HANDLER(&usb_setup_data_reset);

static void usb_setup_device_standard(usb_setup_t pkt)
{
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case GET_DESCRIPTOR: {
			usb_desc_t desc;
			desc.p = usb_ep0_in_buf(&desc.size);
			usb_get_descriptor(&desc, pkt);
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
	case USB_SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SET_ADDRESS:
#if DEBUG
			printf(ESC_DEBUG "%lu\tusb_ep0: USB set address %u\n", systick_cnt(), pkt.wValue);
#endif
			usb_hw_set_addr(pkt.wValue);
			usb_ep0_in_buf(0);
			usb_ep0_in(0);
			break;
		case SET_CONFIGURATION:
			if (pkt.wValue == 1) {
				// Initialise endpoints
				LIST_ITERATE(usb_config, usb_config_handler_t, pfunc) (*pfunc)(pkt.wValue);
				usb_ep0_in_buf(0);
				usb_ep0_in(0);
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

void usb_ep0_iface_register(uint8_t iface, usb_ep0_iface_setup_t setup)
{
	data.iface[iface].setup = setup;
}

static void usb_setup_interface(usb_setup_t pkt)
{
	usb_desc_t desc;
	desc.p = usb_ep0_in_buf(&desc.size);
	if (data.iface[pkt.wIndex].setup) {
		(*data.iface[pkt.wIndex].setup)(pkt, &desc);
	} else {
		USB_TODO();
		desc.size = (uint32_t)-1;
	}
	if (desc.size == (uint32_t)-1)
		usb_ep0_in_stall();
	else
		usb_ep0_in(desc.size);
}

void usb_ep0_setup(void *p, uint32_t size)
{
	usb_setup_t pkt = *(usb_setup_t *)p;
	if (size != 8)
		USB_TODO();

	// Process setup packet
	switch (pkt.bmRequestType & RCPT_Msk) {
	case RCPT_DEVICE:
		if ((pkt.bmRequestType & USB_SETUP_TYPE_TYPE_Msk) == USB_SETUP_TYPE_TYPE_STD) {
			usb_setup_device_standard(pkt);
		} else {
			USB_TODO();
			usb_ep0_in_stall();
		}
		break;
	case RCPT_INTERFACE:
		usb_setup_interface(pkt);
		break;
	case RCPT_ENDPOINT:
		USB_TODO();
		usb_ep0_in_stall();
		break;
	default:
		USB_TODO();
		usb_ep0_in_stall();
	}
}

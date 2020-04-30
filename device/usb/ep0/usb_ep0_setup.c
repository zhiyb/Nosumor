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

#define STATUS_REMOTE_WAKEUP	0b10
#define STATUS_SELF_POWERED	0b01
#define STATUS_DEVICE		0

LIST(usb_config, usb_config_handler_t);

static struct {
	struct {
		usb_ep0_iface_setup_t fsetup;
	} iface[USB_MAX_IFACE];
	usb_setup_t setup;
	usb_ep0_status_t fstatus;
} data;

void usb_setup_data_reset()
{
	for (int i = 0; i < USB_MAX_IFACE; i++)
		data.iface[i].fsetup = 0;
	data.fstatus = 0;
}

USB_RESET_HANDLER(&usb_setup_data_reset);

void usb_ep0_status_register(usb_setup_t pkt, usb_ep0_status_t fstatus)
{
	data.setup = pkt;
	data.fstatus = fstatus;
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_ep0: USB status stage registered %p\n", systick_cnt(), fstatus);
#endif
}

static inline void usb_setup_get_descriptor(usb_setup_t pkt)
{
	usb_desc_t desc;
	desc.p = usb_ep0_in_buf(&desc.size);
	usb_get_descriptor(&desc, pkt);
	if (desc.size != 0)
		usb_ep0_in(desc.size > pkt.wLength ? pkt.wLength : desc.size);
	else
		USB_ERROR();
}

static inline void usb_setup_get_status(usb_setup_t pkt)
{
	if (pkt.wValue != 0 || pkt.wIndex != 0 || pkt.wLength != 2) {
		//usb_ep_in_stall(usb->base, ep);
		USB_ERROR();
		return;
	}

	usb_desc_t desc;
	desc.p = usb_ep0_in_buf(&desc.size);
	*((uint16_t *)desc.p) = STATUS_DEVICE;
	usb_ep0_in(pkt.wLength);
}

static void usb_setup_device_standard(usb_setup_t pkt)
{
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case USB_SETUP_GET_DESCRIPTOR:
			usb_setup_get_descriptor(pkt);
			break;
		case USB_SETUP_GET_STATUS:
			usb_setup_get_status(pkt);
			break;
		default:
			//usb_ep_in_stall(usb->base, ep);
			USB_TODO();
		}
		break;
	case USB_SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case USB_SETUP_SET_ADDRESS:
#if DEBUG
			printf(ESC_DEBUG "%lu\tusb_ep0: USB set address %u\n", systick_cnt(), pkt.wValue);
#endif
			usb_hw_set_addr(pkt.wValue);
			usb_ep0_in_buf(0);
			usb_ep0_in(0);
			break;
		case USB_SETUP_SET_CONFIGURATION:
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

void usb_ep0_iface_register(uint8_t iface, usb_ep0_iface_setup_t fsetup)
{
	data.iface[iface].fsetup = fsetup;
}

static void usb_setup_interface(usb_setup_t pkt)
{
	usb_desc_t desc;
	desc.p = usb_ep0_in_buf(&desc.size);
	if (data.iface[pkt.bID].fsetup) {
		(*data.iface[pkt.bID].fsetup)(pkt, &desc);
	} else {
		USB_TODO();
		desc.size = (uint32_t)-1;
	}
	if (data.fstatus)
		usb_ep0_in_free();
	else if (desc.size == (uint32_t)-1)
		usb_ep0_in_stall();
	else
		usb_ep0_in(desc.size);
}

void usb_ep0_setup(const void *p, uint32_t size)
{
	usb_setup_t pkt = *(const usb_setup_t *)p;
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

void usb_ep0_status(const void *p, uint32_t size)
{
	if (!data.fstatus) {
		usb_ep0_in_stall();
		return;
	}
	size = (*data.fstatus)(data.setup, p, size);
	data.fstatus = 0;
	if (size == 0) {
		usb_ep0_in_buf(0);
		usb_ep0_in(0);
	} else {
		usb_ep0_in_stall();
	}
}

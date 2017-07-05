#include <malloc.h>
#include <debug.h>
#include "usb_keyboard.h"
#include "usb_keyboard_desc.h"
#include "../usb_ram.h"
#include "../usb_desc.h"
#include "../usb_ep.h"
#include "../usb_macros.h"
#include "../usb_setup.h"

#define SETUP_DESC_TYPE_HID		0x21u
#define SETUP_DESC_TYPE_REPORT		0x22u
#define SETUP_DESC_TYPE_PHYSICAL	0x23u

#define SETUP_REQ_GET_REPORT	0x01u
#define SETUP_REQ_GET_IDLE	0x02u
#define SETUP_REQ_GET_PROTOCOL	0x03u
#define SETUP_REQ_SET_REPORT	0x09u
#define SETUP_REQ_SET_IDLE	0x0au
#define SETUP_REQ_SET_PROTOCOL	0x0bu

typedef struct data_t {
	int ep_in;
	uint8_t report[KEYBOARD_REPORT_SIZE];
} data_t;

static void epin_init(usb_t *usb, uint32_t ep)
{
	uint32_t size = KEYBOARD_REPORT_SIZE, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[ep - 1] = DIEPTXF(addr, size);
}

static void ep_halt(struct usb_t *usb, uint32_t ep, int halt)
{
	data_t *p = (data_t *)usb->epin[ep].data;
	EP_IN(usb->base, ep)->DIEPCTL = USB_OTG_DIEPCTL_USBAEP_Msk |
			EP_IN_TYP_INTERRUPT | (ep << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(KEYBOARD_REPORT_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
	if (!halt)
		usb_ep_in_transfer(usb->base, ep, p->report, KEYBOARD_REPORT_SIZE);
}

static void usbif_config(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		.init = epin_init,
		.halt = ep_halt,
	};
	usb_ep_register(usb, &epin, &p->ep_in, 0, 0);

	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	usb_desc_add_interface(usb, 1u, 3u, 0u, 1u, 0u);
	usb_desc_add(usb, &desc_hid, desc_hid.bLength);
	usb_desc_add_endpoint(usb, EP_DIR_IN | p->ep_in,
			      EP_INTERRUPT, KEYBOARD_REPORT_SIZE, 1u);
}

static void set_idle(uint8_t duration, uint8_t reportID)
{
	if (duration != 0)
		dbgbkpt();
}

static void usb_send_descriptor(usb_t *usb, uint32_t ep, setup_t pkt)
{
	cnost_desc_t desc;
	switch (pkt.bType) {
	case SETUP_DESC_TYPE_REPORT:
		desc.p = desc_report;
		desc.size = sizeof(desc_report);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		return;
	}
	desc.size = desc.size > pkt.wLength ? pkt.wLength : desc.size;
	usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
}

static void usbif_setup_std(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
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

static void usbif_setup_class(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_Msk) {
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_REPORT:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		case SETUP_REQ_SET_IDLE:
			set_idle(pkt.bType, pkt.bIndex);
			usb_ep_in_transfer(usb->base, ep, 0, 0);
			break;
		case SETUP_REQ_SET_PROTOCOL:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
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

static void usbif_enable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	ep_halt(usb, p->ep_in, 0);
}

void usb_keyboard_init(usb_t *usb)
{
	usb_if_t usbif = {
		.data = calloc(1u, sizeof(data_t)),
		.config = &usbif_config,
		.setup_std = &usbif_setup_std,
		.setup_class = &usbif_setup_class,
		.enable = &usbif_enable,
	};
	usb_interface_alloc(usb, &usbif);
}

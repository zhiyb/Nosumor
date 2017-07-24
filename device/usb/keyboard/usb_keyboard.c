#include <malloc.h>
#include <string.h>
#include "keyboard.h"
#include "usb_keyboard.h"
#include "usb_keyboard_desc.h"
#include "../usb_debug.h"
#include "../usb_ram.h"
#include "../usb_desc.h"
#include "../usb_ep.h"
#include "../usb_macros.h"
#include "../usb_setup.h"

#define SETUP_DESC_TYPE_HID		0x21
#define SETUP_DESC_TYPE_REPORT		0x22
#define SETUP_DESC_TYPE_PHYSICAL	0x23

#define SETUP_REQ_GET_REPORT	0x01
#define SETUP_REQ_GET_IDLE	0x02
#define SETUP_REQ_GET_PROTOCOL	0x03
#define SETUP_REQ_SET_REPORT	0x09
#define SETUP_REQ_SET_IDLE	0x0a
#define SETUP_REQ_SET_PROTOCOL	0x0b

#define SETUP_REPORT_INPUT	0x01
#define SETUP_REPORT_OUTPUT	0x02
#define SETUP_REPORT_FEATURE	0x03

uint8_t keycodes[KEYBOARD_KEYS] = {
	// x,    z,    c,    ~,  ESC
	0x1b, 0x1d, 0x06, 0x35, 0x29,
};

typedef struct data_t {
	int ep_in;
	int pending;
	uint8_t report[KEYBOARD_REPORT_SIZE];
} data_t;

static struct {
	usb_t *usb;
	data_t *data;
} _usb = {0, 0};

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = KEYBOARD_REPORT_SIZE, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = EP_IN_TYP_INTERRUPT | (n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(KEYBOARD_REPORT_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
}

static void epin_halt(struct usb_t *usb, uint32_t n, int halt)
{
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	uint32_t ctl = ep->DIEPCTL;
	if (!(ctl & USB_OTG_DIEPCTL_USBAEP_Msk) != !halt)
		return;
	if (halt) {
		dbgprintf(ESC_BLUE "HID Keyboard disabled\n");
		if (ctl & USB_OTG_DIEPCTL_EPENA_Msk) {
			DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPDIS_Msk);
			while (ep->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk);
		}
		DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk);
		ep->DIEPCTL &= ~USB_OTG_DIEPCTL_USBAEP_Msk;
		return;
	}
	dbgprintf(ESC_BLUE "HID Keyboard enabled\n");
	DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_USBAEP_Msk);
	//usb_keyboard_update(keyboard_status());
}

static void epin_xfr_cplt(usb_t *usb, uint32_t n)
{
	__disable_irq();
	data_t *data = (data_t *)usb->epin[n].data;
	uint32_t pending = data->pending;
	data->pending = 0;
	__enable_irq();
	if (pending)
		usb_ep_in_transfer(usb->base, n, data->report, KEYBOARD_REPORT_SIZE);
}

static void usbif_config(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		.init = &epin_init,
		.halt = &epin_halt,
		.xfr_cplt = &epin_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &p->ep_in, 0, 0);

	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "HID Keyboard");
	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	usb_desc_add_interface(usb, 0u, 1u, 3u, 0u, 1u, s);
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
	const_desc_t desc;
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

static void usb_send_report(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bType) {
	case SETUP_REPORT_INPUT:
		usb_ep_in_transfer(usb->base, ep, &data->report, KEYBOARD_REPORT_SIZE);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		return;
	}
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
	case SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case SETUP_REQ_GET_REPORT:
			usb_send_report(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_REPORT:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
			break;
		case SETUP_REQ_SET_IDLE:
			set_idle(pkt.bType, pkt.bIndex);
			usb_ep_in_transfer(usb->base, ep, 0, 0);
			break;
		case SETUP_REQ_SET_PROTOCOL:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	}
}

static void usbif_enable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	_usb.usb = usb;
	_usb.data = p;
	epin_halt(usb, p->ep_in, 0);
}

static void usbif_disable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	_usb.usb = 0;
	epin_halt(usb, p->ep_in, 1);
}

void usb_keyboard_init(usb_t *usb)
{
	usb_if_t usbif = {
		.data = calloc(1u, sizeof(data_t)),
		.config = &usbif_config,
		.enable = &usbif_enable,
		.disable = &usbif_disable,
		.setup_std = &usbif_setup_std,
		.setup_class = &usbif_setup_class,
	};
	_usb.usb = 0;
	usb_interface_alloc(usb, &usbif);
}

void usb_keyboard_update(uint32_t status)
{
	if (!_usb.usb)
		return;
	memset(_usb.data->report, 0, KEYBOARD_REPORT_SIZE);
	_usb.data->report[0] = HID_KEYBOARD;	// Report ID
	// Modifier keys, reserved, keycodes
	uint8_t *p = &_usb.data->report[3];
	// Update report
	const uint32_t *pm = keyboard_masks;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++)
		if (status & *pm++)
			*p++ = keycodes[i];
	// Send report
	if (_usb.data->pending)
		return;
	__disable_irq();
	if (usb_ep_in_active(_usb.usb->base, _usb.data->ep_in)) {
		_usb.data->pending = 1;
		__enable_irq();
		return;
	}
	__enable_irq();
	usb_ep_in_transfer(_usb.usb->base, _usb.data->ep_in,
			   _usb.data->report, KEYBOARD_REPORT_SIZE);
}

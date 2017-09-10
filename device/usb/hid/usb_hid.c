#include <malloc.h>
#include <string.h>
#include "../../debug.h"
#include "../usb_ram.h"
#include "../usb_desc.h"
#include "../usb_ep.h"
#include "../usb_macros.h"
#include "../usb_setup.h"
#include "usb_hid.h"

#define HID_IN_MAX_SIZE		48u
#define HID_OUT_MAX_PKT		4u
#define HID_OUT_MAX_SIZE	48u

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

typedef struct PACKED desc_hid_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;
	uint8_t bClassDescriptorType;
	uint16_t wDescriptorLength;
} desc_hid_t;

static const desc_hid_t desc_hid = {
	// 0x21: HID descriptor
	// 0x22: Report descriptor
	9u, 0x21u, 0x0111u, 0x00u, 1u, 0x22u, 0u,
};

typedef struct usb_hid_data_t {
	usb_t *usb;
	usb_hid_t *hid;
	desc_hid_t desc_hid;
	desc_t desc_report;
	int usages, ep_in, ep_out;
	union {
		usb_hid_report_t report;
		uint8_t data[HID_OUT_MAX_SIZE];
	} pktbuf, buf[HID_OUT_MAX_PKT];
} usb_hid_data_t;

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = HID_IN_MAX_SIZE, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = EP_TYP_INTERRUPT | (n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(HID_IN_MAX_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
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
	usb_hid_data_t *data = (usb_hid_data_t *)usb->epin[n].data;
	for (usb_hid_t **hid = &data->hid; *hid != 0; hid = &(*hid)->next)
		if ((*hid)->pending) {
			(*hid)->pending = 0;
			usb_ep_in_transfer(usb->base, n, (*hid)->report.raw, (*hid)->size);
			break;
		}
}

static void epout_init(usb_t *usb, uint32_t n)
{
	usb_hid_data_t *data = usb->epout[n].data;
	// Set endpoint type
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	ep->DOEPCTL = USB_OTG_DOEPCTL_USBAEP_Msk | EP_TYP_INTERRUPT | HID_OUT_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
	// Receive packets
	usb_ep_out_transfer(usb->base, n, data->buf, 0u, HID_OUT_MAX_PKT, HID_OUT_MAX_SIZE);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	usb_hid_data_t *data = usb->epout[n].data;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Calculate packet size
	uint32_t siz = ep->DOEPTSIZ;
	uint32_t pkt_cnt = HID_OUT_MAX_PKT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = HID_OUT_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	// Copy packet
	if (pkt_cnt != 1u)
		dbgbkpt();
	memcpy(data->pktbuf.data, data->buf, size);
	// Receive new packets
	usb_ep_out_transfer(usb->base, n, data->buf, 0u, HID_OUT_MAX_PKT, HID_OUT_MAX_SIZE);
	// Process packet
	for (usb_hid_t **hid = &data->hid; *hid != 0; hid = &(*hid)->next) {
		if ((*hid)->report.id == data->pktbuf.report.id) {
			(*hid)->recv(*hid, &data->pktbuf.report, size);
			return;
		}
	}
}

static void usbif_config(usb_t *usb, void *p)
{
	usb_hid_data_t *data = (usb_hid_data_t *)p;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		.init = &epin_init,
		.halt = &epin_halt,
		.xfr_cplt = &epin_xfr_cplt,
	};
	const epout_t epout = {
		.data = data,
		.init = &epout_init,
		.xfr_cplt = &epout_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &data->ep_in, &epout, &data->ep_out);

	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	usb_desc_add_interface(usb, 0u, 2u, 3u, 1u, 0u, 0);
	usb_desc_add(usb, &data->desc_hid, data->desc_hid.bLength);
	usb_desc_add_endpoint(usb, EP_DIR_IN | data->ep_in,
			      EP_INTERRUPT, HID_IN_MAX_SIZE, 1u);
	usb_desc_add_endpoint(usb, EP_DIR_OUT | data->ep_out,
			      EP_INTERRUPT, HID_IN_MAX_SIZE, 1u);
}

static void set_idle(uint8_t duration, uint8_t reportID)
{
	if (duration != 0)
		dbgbkpt();
}

static void usb_send_descriptor(usb_t *usb, usb_hid_data_t *data, uint32_t ep, setup_t pkt)
{
	desc_t desc;
	switch (pkt.bType) {
	case SETUP_DESC_TYPE_REPORT:
		desc = data->desc_report;
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		return;
	}
	desc.size = desc.size > pkt.wLength ? pkt.wLength : desc.size;
	usb_ep_in_descriptor(usb->base, ep, desc);
}

static void usb_send_report(usb_t *usb, usb_hid_data_t *data, uint32_t ep, setup_t pkt)
{
	uint8_t id = pkt.bIndex;
	switch (pkt.bType) {
	case SETUP_REPORT_INPUT:
		for (usb_hid_t **hid = &data->hid; *hid != 0; hid = &(*hid)->next)
			if ((*hid)->report.id == id) {
				usb_ep_in_transfer(usb->base, ep,
						   (*hid)->report.raw, (*hid)->size);
				return;
			}
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
		return;
	}
}

static void usbif_setup_std(usb_t *usb, void *p, uint32_t ep, setup_t pkt)
{
	usb_hid_data_t *data = (usb_hid_data_t *)p;
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case GET_DESCRIPTOR:
			usb_send_descriptor(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
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

static void usbif_setup_class(usb_t *usb, void *p, uint32_t ep, setup_t pkt)
{
	usb_hid_data_t *data = (usb_hid_data_t *)p;
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case SETUP_REQ_GET_REPORT:
			usb_send_report(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
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

static void usbif_enable(usb_t *usb, void *p)
{
	usb_hid_data_t *data = (usb_hid_data_t *)p;
	data->usb = usb;
	epin_halt(usb, data->ep_in, 0);
}

static void usbif_disable(usb_t *usb, void *p)
{
	usb_hid_data_t *data = (usb_hid_data_t *)p;
	data->usb = 0;
	for (usb_hid_t **hid = &data->hid; *hid != 0; hid = &(*hid)->next)
		(*hid)->pending = 0;
	epin_halt(usb, data->ep_in, 1);
}

usb_hid_data_t *usb_hid_init(usb_t *usb)
{
	usb_hid_data_t *data = (usb_hid_data_t *)calloc(1u, sizeof(usb_hid_data_t));
	if (!data)
		panic();
	usb_if_t usbif = {
		.data = data,
		.config = &usbif_config,
		.enable = &usbif_enable,
		.disable = &usbif_disable,
		.setup_std = &usbif_setup_std,
		.setup_class = &usbif_setup_class,
	};
	// Copy HID descriptor
	memcpy(&data->desc_hid, &desc_hid, desc_hid.bLength);
	// Register interface
	usb_interface_register(usb, &usbif);
	return usbif.data;
}

void usb_hid_update(usb_hid_t *hid)
{
	int ep_in = hid->hid_data->ep_in;
	usb_t *usb = hid->hid_data->usb;
	if (!usb)
		return;
	if (hid->pending)
		return;
	// Check endpoint available
	__disable_irq();
	hid->pending = usb_ep_in_active(usb->base, ep_in);
	__enable_irq();
	if (hid->pending)
		return;
	// Send report
	usb_ep_in_transfer(usb->base, ep_in, hid->report.raw, hid->size);
}

void usb_hid_register(usb_hid_t *hid, const_desc_t desc_report)
{
	// Allocate report ID
	usb_hid_data_t *data = hid->hid_data;
	hid->report.id = ++data->usages;
	// Append to HID list
	usb_hid_t **hp;
	for (hp = &data->hid; *hp != 0; hp = &(*hp)->next);
	*hp = hid;
	// Allocate additional report descriptor space
	data->desc_report.p = realloc(data->desc_report.p,
				      data->desc_report.size + desc_report.size);
	uint8_t *p = data->desc_report.p + data->desc_report.size;
	// Copy report descriptor
	memcpy(p, desc_report.p, desc_report.size);
	data->desc_report.size += desc_report.size;
	data->desc_hid.wDescriptorLength += desc_report.size;
	// Update report ID
	for (uint32_t i = 0; i != desc_report.size; i++)
		if (*p++ == 0x85) {
			*p = hid->report.id;
			break;
		}
}

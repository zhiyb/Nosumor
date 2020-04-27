#include <string.h>
#include <usb/usb_hw.h>
#include <usb/usb_desc.h>
#include <usb/usb_macros.h>
#include <usb/ep0/usb_ep0_setup.h>
#include "usb_hid.h"

#define HID_IN_MAX_SIZE		64u
#define HID_OUT_MAX_PKT		4u
#define HID_OUT_MAX_SIZE	64u

#define SETUP_STD_GET_DESCRIPTOR	6u
#define SETUP_STD_SET_DESCRIPTOR	7u

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

typedef enum {BufFree = 0, BufActive} buf_state_t;

static struct {
	struct {
		uint32_t num;
		usb_hid_report_t * volatile buf;
	} epin;
	struct {
		uint32_t num;
		struct buf_t {
			void *p;
			volatile uint32_t xfrsize;
			volatile buf_state_t state;
		} buf[2];
		volatile uint32_t buf_queue;
	} epout;
	uint32_t active;
} data;

static void usb_hid_ep_out();
static void usb_hid_ep_out_irq_xfrc(void *p, uint32_t size);

static void usb_hid_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc);

static void usb_hid_set_idle(uint8_t duration, uint8_t reportID);

// Allocate endpoints after enumeration

static void usb_hid_enumeration(uint32_t spd)
{
	UNUSED(spd);
	data.epin.num = usb_hw_ep_alloc(UsbEpNormal, EP_DIR_IN, EP_INTERRUPT, HID_IN_MAX_SIZE);
	data.epout.num = usb_hw_ep_alloc(UsbEpNormal, EP_DIR_OUT, EP_INTERRUPT, HID_OUT_MAX_SIZE);

	// Allocate DMA memories
	data.epin.buf = 0;
	data.epout.buf[0].p = usb_hw_ram_alloc(HID_OUT_MAX_SIZE);
	data.epout.buf[0].state = BufFree;
	data.epout.buf[1].p = usb_hw_ram_alloc(HID_OUT_MAX_SIZE);
	data.epout.buf[1].state = BufFree;
	data.epout.buf_queue = 0;
	data.active = 0;
}

USB_ENUM_HANDLER(&usb_hid_enumeration);

// Disable HID on USB suspend

static void usb_hid_susp()
{
	data.active = 0;
}

USB_SUSP_HANDLER(&usb_hid_susp);

// Enable endpoints on Set Configuration

static void usb_hid_config(uint8_t config)
{
	UNUSED(config);

	usb_hw_ep_out_irq(data.epin.num, 0, 0, &usb_hid_ep_out_irq_xfrc);
	usb_hid_ep_out();
	usb_hw_ep_in_nak(data.epout.num);
	data.active = 1;
}

USB_CONFIG_HANDLER(&usb_hid_config);

// Endpoint handler

static void usb_hid_ep_out()
{
	struct buf_t *buf = &data.epout.buf[data.epout.buf_queue];
#if DEBUG
	if (buf->state != BufFree)
		USB_ERROR();
#endif
	usb_hw_ep_out(data.epout.num, buf->p, 0, 1, 0);
	buf->state = BufActive;
}

static void usb_hid_ep_out_irq_xfrc(void *p, uint32_t size)
{
	USB_TODO();
}

static void usb_hid_ep_in_irq_xfrc(void *p, uint32_t size)
{
	__disable_irq();
	usb_hid_report_t *pxfrc = data.epin.buf;
#if DEBUG >= 5
	if (p != pxfrc->p)
		USB_ERROR();
#endif
	usb_hid_report_t *prpt = pxfrc->next;
	data.epin.buf = prpt;
	__enable_irq();

	if (prpt != 0)
		usb_hw_ep_in(data.epin.num, prpt->p, prpt->len, usb_hid_ep_in_irq_xfrc);
	if (pxfrc->cb)
		(*pxfrc->cb)(pxfrc);
}

// Configuration and report descriptor

USB_HID_DESC_HANDLER_LIST();

static void usb_hid_desc_report(usb_hid_desc_t *pdesc)
{
	LIST_ITERATE(usb_hid_desc, usb_hid_desc_handler_t, pfunc) (*pfunc)(pdesc);
}

static void usb_hid_desc_config(usb_desc_t *pdesc)
{
	usb_hid_desc_t drep;
	uint8_t buf[pdesc->size];
	drep.p = buf;
	drep.len = 0;
	drep.size = pdesc->size;
	usb_hid_desc_report(&drep);

	if (drep.len == 0)
		return;

	desc_hid_t desc;
	desc = desc_hid;
	desc.wDescriptorLength = drep.len;

	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	uint8_t iface = usb_desc_add_interface(pdesc, 0u, 2u, 3u, 1u, 0u, 0);
	usb_ep0_iface_register(iface, &usb_hid_iface_setup);
	usb_desc_add(pdesc, &desc, desc.bLength);
	usb_desc_add_endpoint(pdesc, EP_DIR_IN | data.epin.num, EP_INTERRUPT, HID_IN_MAX_SIZE, 1u);
	usb_desc_add_endpoint(pdesc, EP_DIR_OUT | data.epout.num, EP_INTERRUPT, HID_OUT_MAX_SIZE, 1u);
}

USB_DESC_CONFIG_HANDLER(usb_hid_desc_config);

static inline void usb_hid_desc(usb_setup_t pkt, usb_desc_t *pdesc)
{
	usb_hid_desc_t drep;
	uint8_t buf[pdesc->size];
	drep.p = buf;
	drep.len = 0;
	drep.size = pdesc->size;

	switch (pkt.bType) {
	case SETUP_DESC_TYPE_REPORT:
		usb_hid_desc_report(&drep);
		break;
	default:
		USB_TODO();
		pdesc->size = -1;
		return;
	}

	drep.len = drep.len > pkt.wLength ? pkt.wLength : drep.len;
#if DEBUG >= 5
	if (pdesc->size <= drep.len)
		USB_ERROR();
#endif
	memcpy(pdesc->p, drep.p, drep.len);
	pdesc->size = drep.len;
}

static inline void usb_hid_iface_setup_standard(usb_setup_t pkt, usb_desc_t *pdesc)
{
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case SETUP_STD_GET_DESCRIPTOR:
			usb_hid_desc(pkt, pdesc);
			break;
		default:
			USB_TODO();
			pdesc->size = -1;
		}
		break;
	case USB_SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		default:
			USB_TODO();
			pdesc->size = -1;
		}
		break;
	}
}

static inline void usb_hid_iface_setup_class(usb_setup_t pkt, usb_desc_t *pdesc)
{
	switch (pkt.bmRequestType & USB_SETUP_TYPE_DIR_Msk) {
	case USB_SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case SETUP_REQ_GET_REPORT:
			USB_TODO();
			pdesc->size = -1;
			break;
		default:
			USB_TODO();
			pdesc->size = -1;
		}
		break;
	case USB_SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_REPORT:
			USB_TODO();
			pdesc->size = -1;
			break;
		case SETUP_REQ_SET_IDLE:
			usb_hid_set_idle(pkt.bType, pkt.bIndex);
			pdesc->size = 0;
			break;
		case SETUP_REQ_SET_PROTOCOL:
			USB_TODO();
			pdesc->size = -1;
			break;
		default:
			USB_TODO();
			pdesc->size = -1;
		}
		break;
	}
}

static void usb_hid_iface_setup(usb_setup_t pkt, usb_desc_t *pdesc)
{
	switch (pkt.bmRequestType & USB_SETUP_TYPE_TYPE_Msk) {
	case USB_SETUP_TYPE_TYPE_STD:
		usb_hid_iface_setup_standard(pkt, pdesc);
		break;
	case USB_SETUP_TYPE_TYPE_CLASS:
		usb_hid_iface_setup_class(pkt, pdesc);
		break;
	default:
		USB_TODO();
		pdesc->size = -1;
	}
}

static void usb_hid_set_idle(uint8_t duration, uint8_t reportID)
{
	if (duration != 0)
		USB_TODO();
}

void usb_hid_report_enqueue(usb_hid_report_t *prpt)
{
	if (!data.active) {
		if (prpt->cb)
			(*prpt->cb)(prpt);
		return;
	}

	prpt->next = 0;
	__disable_irq();
	uint32_t idle = data.epin.buf == 0;
	usb_hid_report_t **p = (usb_hid_report_t **)&data.epin.buf;
	while (*p)
		p = (usb_hid_report_t **)&((*p)->next);
	*p = prpt;
	__enable_irq();

	if (idle)
		usb_hw_ep_in(data.epin.num, prpt->p, prpt->len, usb_hid_ep_in_irq_xfrc);
}

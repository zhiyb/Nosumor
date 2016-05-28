#include "usb_def.h"
#include "usb_desc.h"
#include "usb_ep0.h"
#include "usb_class.h"
#include "usb_debug.h"

#define HID_REQ_GET_REPORT	0x01
#define HID_REQ_GET_IDLE	0x02
#define HID_REQ_GET_PROTOCOL	0x03
#define HID_REQ_SET_REPORT	0x09
#define HID_REQ_SET_IDLE	0x0a
#define HID_REQ_SET_PROTOCOL	0x0b

__IO uint32_t ep1rx[EP1_SIZE] __attribute__((section(".usbram")));

void usbClassInit()
{
	eptable[1][EP_RX].addr = USB_LOCAL_ADDR(&ep1rx);
	eptable[1][EP_RX].count = USB_RX_COUNT_REG(sizeof(ep1rx) / 2);
}

void usbClassReset()
{
	// Configure endpoint 1
	USB->EP1R = USB_EP_INTERRUPT | 1;
}

void usbClassHalt(uint16_t epaddr, uint16_t e)
{
	if (e)
		writeString("[HALT_");
	else
		writeString("[VALID_");
	dumpHex(epaddr);
	writeChar(']');

	uint16_t epid = epaddr & EP_ADDR_MASK;
	switch (epid) {
	case 1:
		if (e)
			usbStall(epid, EP_RX);
		else
			usbValid(epid, EP_RX);
		return;
	default:
		dbbkpt();
	}
}

void usbClassSetup(struct setup_t *setup)
{
	switch (setup->type & TYPE_RCPT_MASK) {
	case TYPE_RCPT_INTERFACE:
		switch (setup->request) {
		case HID_REQ_GET_REPORT:
			if (!(setup->type & TYPE_DIRECTION))
				goto error;
			// Mandatory, not implemented
			usbStall(0, EP_TX);
			dbbkpt();
			break;
		case HID_REQ_SET_IDLE:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			// Unsupported
			writeString("[S_IDLE]");
			usbStall(0, EP_TX);
			return;
		default:
			// Not implemented
			usbStall(0, EP_TX);
			dbbkpt();
			return;
		}
	default:
		// Not implemented
		usbStall(0, EP_TX);
		dbbkpt();
		return;
	}

error:
	usbStall(0, EP_TX);
	dbbkpt();
}

static void getDescriptor(struct setup_t *setup)
{
	writeString("[DESC_");
	uint8_t type = setup->descriptor.type, index = setup->descriptor.index;
	struct ep_t *ep = &eptable[0][EP_TX];
	const void *desc = 0;
	uint32_t size = 0;

	switch (type) {
	case DESC_TYPE_REPORT:
		writeString("REPORT]");
		if (index < descriptors.report.num) {
			desc = descriptors.report.list[index].data;
			size = descriptors.report.list[index].size;
		}
		break;
	}

	size = size < setup->length ? size : setup->length;
	if (desc)
		usbTransfer(0, EP_TX, desc, ep->addr, size);
	else {
		usbStall(0, EP_TX);
		dbbkpt();
	}
}

void usbClassSetupInterface(struct setup_t *setup)
{
	switch (setup->request) {
	case REQ_GET_DESCRIPTOR:
		if (!(setup->type & TYPE_DIRECTION)) {
			usbStall(0, EP_TX);
			dbbkpt();
			break;
		}
		getDescriptor(setup);
		break;
	default:
		usbStall(0, EP_TX);
		dbbkpt();
	}
}

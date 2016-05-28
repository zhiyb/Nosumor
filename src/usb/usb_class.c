#include "usb_def.h"
#include "usb_desc.h"
#include "usb_ep0.h"
#include "usb_class.h"
#include "usb_debug.h"

#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_GET_IDLE		0x02
#define HID_REQ_GET_PROTOCOL	0x03
#define HID_REQ_SET_REPORT		0x09
#define HID_REQ_SET_IDLE		0x0a
#define HID_REQ_SET_PROTOCOL	0x0b

void usbClassSetup(struct setup_t *setup)
{
	switch (setup->type & TYPE_RCPT_MASK) {
	case TYPE_RCPT_INTERFACE:
		switch (setup->request) {
		case HID_REQ_GET_REPORT:
			if (!(setup->type & TYPE_DIRECTION)) {
				usbStall(0, EP_TX);
				dbbkpt();
				break;
			}
			// Mandatory
			usbStall(0, EP_TX);
			dbbkpt();
			break;
		case HID_REQ_SET_IDLE:
			if (setup->type & TYPE_DIRECTION) {
				usbStall(0, EP_TX);
				dbbkpt();
				break;
			}
			writeString("[S_IDLE]");
			// Unsupported
			usbStall(0, EP_TX);
			break;
		default:
			usbStall(0, EP_TX);
			dbbkpt();
		}
		break;
	default:
		usbStall(0, EP_TX);
		dbbkpt();
	}
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
		usbStall(0, EP_TX);
		dbbkpt();
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

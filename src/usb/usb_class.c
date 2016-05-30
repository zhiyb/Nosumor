#include "usb.h"
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

__IO uint32_t ep1rx[EP1_SIZE / 2] USBRAM;
uint32_t ep1tx[EP1_SIZE / 2] USBRAM;

void usbClassInit()
{
	eptable[1][EP_TX].addr = USB_LOCAL_ADDR(&ep1tx);
	eptable[1][EP_TX].count = 0;
	eptable[1][EP_RX].addr = USB_LOCAL_ADDR(&ep1rx);
	eptable[1][EP_RX].count = USB_RX_COUNT_REG(EP1_SIZE);
}

void usbClassReset()
{
	// Configure endpoint 1
	USB->EP1R = USB_EP_INTERRUPT | USB_EP_RX_DIS | USB_EP_TX_NAK | 1;
	memset(ep1tx, 0, sizeof(ep1tx));
}

void usbClassHalt(uint16_t epaddr, uint16_t e)
{
	if (e)
		writeString("[H");
	else
		writeString("[V");
	dumpHex(epaddr);
	writeChar(']');

	uint16_t epid = epaddr & EP_ADDR_MASK;
	switch (epid) {
	case 1:
		if (e)
			usbDisable(epid, EP_TX);
		else
			usbNAK(epid, EP_TX);
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
	uint8_t type = setup->descriptor.type, index = setup->descriptor.index;
	const void *desc = 0;
	uint32_t size = 0;

	switch (type) {
	case DESC_TYPE_REPORT:
		writeString("[REPORT]");
		if (index < descriptors.report.num) {
			desc = descriptors.report.list[index].data;
			size = descriptors.report.list[index].size;
		}
		break;
	}

	size = size < setup->length ? size : setup->length;
	if (desc)
		usbTransfer(0, EP_TX, EP0_SIZE, size, desc);
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
		writeString("[G_DESC]");
		getDescriptor(setup);
		break;
	default:
		// Not implemented
		usbStall(0, EP_TX);
		dbbkpt();
	}
}

void usbHIDReport(const void *ptr, uint8_t size)
{
	usbTransfer(1, EP_TX, EP1_SIZE, size, ptr);
}

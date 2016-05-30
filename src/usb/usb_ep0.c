#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_desc.h"
#include "usb_class.h"
#include "usb_ep0.h"
#include "usb_debug.h"

#define TYPE_TYPE_MASK		0x60
#define TYPE_TYPE_STANDARD	0x00
#define TYPE_TYPE_CLASS		0x20
#define TYPE_TYPE_VENDOR	0x40

#define REQ_GET_STATUS		0
#define REQ_CLEAR_FEATURE	1
#define REQ_SET_FEATURE		3
#define REQ_SET_ADDRESS		5
#define REQ_GET_CONFIGURATION	8
#define REQ_SET_CONFIGURATION	9
#define REQ_GET_INTERFACE	10
#define REQ_SET_INTERFACE	11
#define REQ_SYNCH_FRAME		12

#define FS_ENDPOINT_HALT	0
#define FS_DEVICE_REMOTE_WAKEUP	1
#define FS_TEST_MODE		2

__IO uint32_t ep0rx[EP0_SIZE / 2] USBRAM;
uint32_t ep0tx[EP0_SIZE / 2] USBRAM;

void usbEP0Init()
{
	eptable[0][EP_TX].addr = USB_LOCAL_ADDR(ep0tx);
	eptable[0][EP_TX].count = 0;
	eptable[0][EP_RX].addr = USB_LOCAL_ADDR(&ep0rx);
	eptable[0][EP_RX].count = USB_RX_COUNT_REG(EP0_SIZE);
}

void usbEP0Reset()
{
	// Configure endpoint 0
	USB->EP0R = USB_EP_CONTROL | USB_EP_RX_VALID | USB_EP_TX_NAK | 0;
}

static void setConfiguration(uint8_t config)
{
	if (config != 1) {
		usbStall(0, EP_TX);
		dbbkpt();
	} else {
		usbStatus.config = config;
		usbTransferEmpty(0, EP_TX);
	}
}

static void clearFeature(struct setup_t *setup)
{
	if (!usbStatus.config)
		goto error;

	switch (setup->value) {
	case FS_ENDPOINT_HALT:
		writeString("{C_HALT}");
		if ((setup->type & TYPE_RCPT_MASK) != TYPE_RCPT_ENDPOINT)
			goto error;
		if ((setup->index & EP_ADDR_MASK) == 0)
			goto error;
		usbClassHalt(setup->index, 0);
		usbTransferEmpty(0, EP_TX);
		return;
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
	case DESC_TYPE_DEVICE:
		writeString("{DEV}");
		if (index < descriptors.device.num) {
			desc = descriptors.device.list[index].data;
			size = descriptors.device.list[index].size;
		}
		break;
	case DESC_TYPE_CONFIG:
		writeString("{CFG}");
		if (index < descriptors.config.num) {
			desc = descriptors.config.list[index].data;
			size = descriptors.config.list[index].size;
		}
		break;
	case DESC_TYPE_DEV_QUALIFIER:
		writeString("{QUALIFIER}");
		// Full speed only device
		usbStall(0, EP_TX);
		return;
	}

	size = size < setup->length ? size : setup->length;
	if (desc)
		usbTransfer(0, EP_TX, EP0_SIZE, size, desc);
	else {
		usbStall(0, EP_TX);
		dbbkpt();
	}
}

static void standardSetup(struct setup_t *setup)
{
	switch (setup->type & TYPE_RCPT_MASK) {
	case TYPE_RCPT_DEVICE:
		switch (setup->request) {
		case REQ_CLEAR_FEATURE:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			clearFeature(setup);
			return;
		case REQ_GET_DESCRIPTOR:
			if (!(setup->type & TYPE_DIRECTION))
				goto error;
			writeString("{G_DESC}");
			getDescriptor(setup);
			return;
		case REQ_SET_ADDRESS:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			writeString("{S_ADDR}");
			usbTransferEmpty(0, EP_TX);
			usbStatus.addr = setup->value & 0x7f;
			return;
		case REQ_SET_CONFIGURATION:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			writeString("{S_CONF}");
			setConfiguration(setup->value);
			return;
		default:
			// Not implemented
			usbStall(0, EP_TX);
			dbbkpt();
			return;
		}
	case TYPE_RCPT_INTERFACE:
		switch (setup->request) {
		case REQ_CLEAR_FEATURE:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			clearFeature(setup);
			return;
		case REQ_GET_INTERFACE:
		case REQ_GET_STATUS:
		case REQ_SET_FEATURE:
		case REQ_SET_INTERFACE:
			// Not implemented
			usbStall(0, EP_TX);
			dbbkpt();
			return;
		default:
			usbClassSetupInterface(setup);
			return;
		}
	case TYPE_RCPT_ENDPOINT:
		switch (setup->request) {
		case REQ_CLEAR_FEATURE:
			if (setup->type & TYPE_DIRECTION)
				goto error;
			clearFeature(setup);
			return;
		case REQ_GET_STATUS:
		case REQ_SET_FEATURE:
		case REQ_SYNCH_FRAME:
			// Not implemented
			usbStall(0, EP_TX);
			dbbkpt();
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

void usbEP0Setup()
{
	struct ep_t *ep = &eptable[0][EP_RX];
	struct setup_t *setup = (struct setup_t *)USB_SYS_ADDR(ep->addr);
	switch (setup->type & TYPE_TYPE_MASK) {
	case TYPE_TYPE_STANDARD:
		writeString("{STD}");
		standardSetup(setup);
		break;
	case TYPE_TYPE_CLASS:
		writeString("{CLASS}");
		usbClassSetup(setup);
		break;
	default:
		usbStall(0, EP_TX);
		//dbbkpt();
	}
}

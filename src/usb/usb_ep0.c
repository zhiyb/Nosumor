#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
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

__IO uint32_t ep0rx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));
uint32_t ep0tx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));

void usbInitEP0()
{
	eptable[0][EP_TX].addr = USB_LOCAL_ADDR(ep0tx);
	eptable[0][EP_TX].count = 0;
	eptable[0][EP_RX].addr = USB_LOCAL_ADDR(&ep0rx);
	eptable[0][EP_RX].count = USB_RX_COUNT_REG(sizeof(ep0rx) / 2);
}

void usbResetEP0()
{
	// Configure endpoint 0
	USB->EP0R = USB_EP_CONTROL | USB_EP_RX_VALID | USB_EP_TX_VALID | 0;
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

static void getDescriptor(struct setup_t *setup)
{
	writeString("{DESC_");
	uint8_t type = setup->descriptor.type, index = setup->descriptor.index;
	struct ep_t *ep = &eptable[0][EP_TX];
	const void *desc = 0;
	uint32_t size = 0;

	switch (type) {
	case DESC_TYPE_DEVICE:
		writeString("DEV}");
		if (index < descriptors.device.num) {
			desc = descriptors.device.list[index].data;
			size = descriptors.device.list[index].size;
		}
		break;
	case DESC_TYPE_CONFIG:
		writeString("CFG}");
		if (index < descriptors.config.num) {
			desc = descriptors.config.list[index].data;
			size = descriptors.config.list[index].size;
		}
		break;
	case DESC_TYPE_DEV_QUALIFIER:
		writeString("QUALIFIER}");
		// Full speed only device
		usbStall(0, EP_TX);
		return;
	}

	size = size < setup->length ? size : setup->length;
	if (desc)
		usbTransfer(0, EP_TX, desc, ep->addr, size);
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
		case REQ_GET_DESCRIPTOR:
			if (!(setup->type & TYPE_DIRECTION)) {
				usbStall(0, EP_TX);
				dbbkpt();
				break;
			}
			getDescriptor(setup);
			break;
		case REQ_SET_ADDRESS:
			if (setup->type & TYPE_DIRECTION) {
				usbStall(0, EP_TX);
				dbbkpt();
				break;
			}
			writeString("{ADDR}");
			usbTransferEmpty(0, EP_TX);
			usbStatus.addr = setup->value & 0x7f;
			break;
		case REQ_SET_CONFIGURATION:
			if (setup->type & TYPE_DIRECTION) {
				usbStall(0, EP_TX);
				dbbkpt();
				break;
			}
			setConfiguration(setup->value);
			break;
		default:
			usbStall(0, EP_TX);
			dbbkpt();
		}
		break;
	case TYPE_RCPT_INTERFACE:
		switch (setup->request) {
		default:
			usbClassSetupInterface(setup);
		}
		break;
	default:
		usbStall(0, EP_TX);
		dbbkpt();
	}
}

void usbSetupEP0()
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
		dbbkpt();
	}
}

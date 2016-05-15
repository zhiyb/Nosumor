#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
#include "debug.h"

extern uint32_t _susbram;

static void usbGetDescriptor(uint32_t epid, uint16_t value)
{
	uint8_t type = value >> 8, index = value & 0xff;
	struct ep_t *ep = &eptable[epid][EP_TX];
	const void *desc = 0;
	uint32_t size = 0;
	switch (type) {
	case DESC_TYPE_DEVICE:
		desc = descriptors.device[index].data;
		size = descriptors.device[index].size;
		break;
	case DESC_TYPE_CONFIG:
		desc = descriptors.config[index].data;
		size = descriptors.config[index].size;
		break;
	}
	if (desc)
		usbTransfer(epid, EP_TX, desc, ep->addr, size);
	else
		dbbkpt();
}

void usbSetup(uint32_t epid)
{
	uint32_t bkpt = 1;
	struct ep_t *ep = &eptable[epid][EP_RX];
	struct setup_t *setup = USB_SYS_ADDR(ep->addr);
	if (setup->type & TYPE_DIRECTION) {
		switch (setup->type & TYPE_TYPE_MASK) {
		case TYPE_TYPE_STANDARD:
			switch (setup->type & TYPE_RCPT_MASK) {
			case TYPE_RCPT_DEVICE:
				switch (setup->request) {
				case REQ_GET_DESCRIPTOR:
					usbGetDescriptor(epid, setup->value);
					bkpt = 0;
					break;
				}
			}
		}
	} else {
		switch (setup->type & TYPE_TYPE_MASK) {
		case TYPE_TYPE_STANDARD:
			switch (setup->type & TYPE_RCPT_MASK) {
			case TYPE_RCPT_DEVICE:
				switch (setup->request) {
				case REQ_SET_ADDRESS:
					usbTransfer(epid, EP_TX, 0, ep->addr, 0);
					usbStatus.addr = setup->value;
					bkpt = 0;
					break;
				}
			}
		}
	}
	if (bkpt)
		dbbkpt();
}

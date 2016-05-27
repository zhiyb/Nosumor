#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
#include "usart1.h"
#include "debug.h"

extern uint32_t _susbram;

static void setConfiguration(uint8_t config)
{
	if (config != 1)
		dbbkpt();
	usbStatus.config = config;
}

static void usbGetDescriptor(uint16_t epid, struct setup_t *setup)
{
	usart1WriteString("[DESC]");
	uint8_t type = setup->value >> 8, index = setup->value & 0xff;
	struct ep_t *ep = &eptable[epid][EP_TX];
	const void *desc = 0;
	uint32_t size = 0;
	switch (type) {
	case DESC_TYPE_DEVICE:
		usart1WriteString("[DEV]");
		desc = descriptors.device[index].data;
		size = descriptors.device[index].size;
		break;
	case DESC_TYPE_CONFIG:
		usart1WriteString("[CFG]");
		desc = descriptors.config[index].data;
		size = descriptors.config[index].size;
		break;
	case DESC_TYPE_DEV_QUALIFIER:
		usart1WriteString("[QUA]");
		// Full speed only device
		usbStall(epid, EP_TX);
		return;
	}
	size = size < setup->length ? size : setup->length;
	usart1DumpHex(size);
	if (desc)
		usbTransfer(epid, EP_TX, desc, ep->addr, size);
	else
		dbbkpt();
}

void usbSetup(uint16_t epid)
{
	usart1WriteString("[SETUP]");
	struct ep_t *ep = &eptable[epid][EP_RX];
	struct setup_t *setup = (struct setup_t *)USB_SYS_ADDR(ep->addr);
	switch (setup->type & TYPE_TYPE_MASK) {
	case TYPE_TYPE_STANDARD:
		switch (setup->type & TYPE_RCPT_MASK) {
		case TYPE_RCPT_DEVICE:
			if (setup->type & TYPE_DIRECTION) {
				switch (setup->request) {
				case REQ_GET_DESCRIPTOR:
					usbGetDescriptor(epid, setup);
					break;
				default:
					dbbkpt();
				}
			} else {
				switch (setup->request) {
				case REQ_SET_ADDRESS:
					usart1WriteString("[ADDR]");
					usbTransfer(epid, EP_TX, 0, ep->addr, 0);
					usbStatus.addr = setup->value & 0x7f;
					break;
				case REQ_SET_CONFIGURATION:
					setConfiguration(setup->value);
					usbTransfer(epid, EP_TX, 0, ep->addr, 0);
					break;
				default:
					dbbkpt();
				}
			}
			break;
		default:
			dbbkpt();
		}
		break;
	case TYPE_TYPE_CLASS:
		switch (setup->type & TYPE_RCPT_MASK) {
		case TYPE_RCPT_INTERFACE:
			if (setup->type & TYPE_DIRECTION) {
				dbbkpt();
			} else {
				switch (setup->request) {
				default:
					dbbkpt();
				}
			}
			break;
		default:
			dbbkpt();
		}
		break;
	default:
		dbbkpt();
	}
}

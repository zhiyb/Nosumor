#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
#include "debug.h"

extern uint32_t _susbram;

static void usbGetDeviceDescriptor()
{
	;
}

void usbSetup(uint32_t epid)
{
	struct ep_t *ep = &eptable[epid][EP_RX];
	struct setup_t *setup = USB_SYS_ADDR(ep->addr);
	if (setup->type & TYPE_DIR) {
		switch (setup->type & TYPE_TYPE_MASK) {
		case TYPE_TYPE_STANDARD:
			switch (setup->type & TYPE_RCPT_MASK) {
			case TYPE_RCPT_DEVICE:
				switch (setup->request) {
				case REQ_GET_DESCRIPTOR:
					usbGetDeviceDescriptor();
					break;
				}
			}
		}
	}
	dbbkpt();
}

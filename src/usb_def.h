#ifndef USB_DEF_H
#define USB_DEF_H

#include <stdint.h>
#include "stm32f1xx.h"

// 0 = Host to Device
// 1 = Device to Host
#define TYPE_DIR		0x80

#define TYPE_TYPE_MASK		0x60
#define TYPE_TYPE_STANDARD	0x00
#define TYPE_TYPE_CLASS		0x20
#define TYPE_TYPE_VENDOR	0x40

#define TYPE_RCPT_MASK		0x1f
#define TYPE_RCPT_DEVICE	0x00
#define TYPE_RCPT_INTERFACE	0x01
#define TYPE_RCPT_ENDPOINT	0x02
#define TYPE_RCPT_OTHER		0x03

#define REQ_GET_STATUS		0x00
#define REQ_CLEAR_FEATURE	0x01
#define REQ_SET_FEATURE		0x03
#define REQ_SET_ADDRESS		0x05
#define REQ_GET_DESCRIPTOR	0x06
#define REQ_SET_DESCRIPTOR	0x07
#define REQ_GET_CONFIGURATION	0x08
#define REQ_SET_CONFIGURATION	0x09

#define USB_LOCAL_ADDR(addr)	(((void *)(addr) - (void *)&_susbram) >> 1)
#define USB_SYS_ADDR(addr)	((void *)&_susbram + ((uint16_t)(addr) << 1))
#define USB_NUM_BLOCK(bl, size)	((bl) ? (size) / 32 : (size) / 2)
#define USB_RX_COUNT(n)		((((n) > 62) << 15) | (USB_NUM_BLOCK(((n) > 62), (n)) << 10))

#define EP_TX	0
#define EP_RX	1

struct ep_t {
	uint16_t addr;
	uint16_t RESERVED0;
	__IO uint16_t count;
	uint16_t RESERVED1;
};

struct setup_t {
	uint8_t type, request;
	uint16_t RESERVED0;
	uint16_t value;
	uint16_t RESERVED1;
	uint16_t index;
	uint16_t RESERVED2;
	uint16_t length;
	uint16_t RESERVED3;
};

union eprx_t {
	struct setup_t setup;
};

extern struct ep_t eptable[8][2] __attribute__((section(".usbtable")));

void usbSetup(uint32_t epid);

#endif // USB_DEF_H

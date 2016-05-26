#ifndef USB_DEF_H
#define USB_DEF_H

#include <stdint.h>
#include "stm32f1xx.h"

#define USB_VID		0x0483
#define USB_PID		0x5750

#define MAX_EP0_SIZE	64

// 0 = Host to Device
// 1 = Device to Host
#define TYPE_DIRECTION		0x80

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

#define DESC_TYPE_DEVICE	0x01
#define DESC_TYPE_CONFIG	0x02
#define DESC_TYPE_INTERFACE	0x04
#define DESC_TYPE_ENDPOINT	0x05
#define DESC_TYPE_DEV_QUALIFIER	0x06
#define DESC_TYPE_HID		0x21
#define DESC_TYPE_REPORT	0x22
#define DESC_TYPE_PHYSICAL	0x23

// Endpoint direction
#define DESC_EP_IN		0x80
#define DESC_EP_OUT		0x00

// Transfer type
#define DESC_EP_CONTROL		0x00
#define DESC_EP_ISOCHRONOUS	0x01
#define DESC_EP_BULK		0x02
#define DESC_EP_INTERRUPT	0x03

// Iso mode synchronisation type
#define DESC_EP_ISO_NONE	0x00
#define DESC_EP_ISO_ASYNC	0x04
#define DESC_EP_ISO_ADAPTIVE	0x08
#define DESC_EP_ISO_SYNC	0x0c

// Iso mode usage type
#define DESC_EP_ISO_DATA	0x00
#define DESC_EP_ISO_FEEDBACK	0x10
#define DESC_EP_ISO_EXPLICIT	0x20

#define USB_LOCAL_ADDR(addr)	((uint16_t)(((uint32_t)(addr) - (uint32_t)&_susbram) >> 1))
#define USB_SYS_ADDR(addr)	((void *)((uint32_t)&_susbram + ((uint32_t)(addr) << 1)))
#define USB_NUM_BLOCK(bl, size)	((bl) ? (size) / 32 : (size) / 2)
#define USB_RX_COUNT(n)		((((n) > 62) << 15) | (USB_NUM_BLOCK(((n) > 62), (n)) << 10))
#define USB_RX_COUNT_MASK	0x03ff
#define USB_RX_NUM(c)		(((c) >> 10) & 0x1f)
#define USB_RX_BL(c)		((c) & 0x8000)

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

struct desc_t {
	const uint32_t size;
	const void *data;
};

struct descriptor_t {
	const struct desc_t *device, *config;
};

struct status_t {
	struct {
		volatile uint8_t active;
		uint8_t epid;
		uint8_t dir;
	} dma;
	uint8_t addr;
};

extern struct ep_t eptable[8][2] __attribute__((section(".usbtable")));
extern __IO uint32_t ep0rx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));
extern uint32_t ep0tx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));
extern const struct descriptor_t descriptors;
extern struct status_t usbStatus;

void usbSetup(uint16_t epid);
void usbTransfer(uint16_t epid, uint16_t dir, const void *src, uint16_t dst, uint32_t size);
void usbHandshake(uint16_t epid, uint16_t dir, uint16_t type);
static inline void usbValid(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_VALID);}
static inline void usbStall(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_STALL);}

#endif // USB_DEF_H

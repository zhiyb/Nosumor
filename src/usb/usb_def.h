#ifndef USB_DEF_H
#define USB_DEF_H

#include <stdint.h>
#include "stm32f1xx.h"

#define USB_LOCAL_ADDR(addr)	((uint16_t)(((uint32_t)(addr) - (uint32_t)&_susbram) >> 1))
#define USB_SYS_ADDR(addr)	((void *)((uint32_t)&_susbram + ((uint32_t)(addr) << 1)))
#define USB_NUM_BLOCK(bl, size)	((bl) ? (size) / 32 : (size) / 2)
#define USB_RX_COUNT_REG(n)	((((n) > 62) << 15) | (USB_NUM_BLOCK(((n) > 62), (n)) << 10))
#define USB_RX_COUNT_MASK	0x03ff
#define USB_RX_NUM(c)		(((c) >> 10) & 0x1f)
#define USB_RX_BL(c)		((c) & 0x8000)

#define EP_TX	0
#define EP_RX	1

struct setup_t;

#include "usb_class.h"

struct ep_t {
	uint16_t addr;
	uint16_t RESERVED0;
	__IO uint16_t count;
	uint16_t RESERVED1;
};

struct setup_value_descriptor_t {
	uint8_t index;
	uint8_t type;
};

struct setup_t {
	uint8_t type, request;
	uint16_t RESERVED0;
	union {
		uint16_t value;
		struct setup_value_descriptor_t descriptor;
		union setup_value_class_t classValue;
	};
	uint16_t RESERVED1;
	uint16_t index;
	uint16_t RESERVED2;
	uint16_t length;
	uint16_t RESERVED3;
};

struct status_t {
	struct {
		volatile uint8_t active;
		uint8_t epid;
		uint8_t dir;
	} dma;
	uint8_t addr, config;
};

extern uint32_t _susbram;
extern struct ep_t eptable[8][2] __attribute__((section(".usbtable")));
extern struct status_t usbStatus;

void usbSetup(uint16_t epid);
void usbTransfer(uint16_t epid, uint16_t dir, const void *src, uint16_t dst, uint32_t size);
void usbTransferEmpty(uint16_t epid, uint16_t dir);
void usbHandshake(uint16_t epid, uint16_t dir, uint16_t type);
static inline void usbValid(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_VALID);}
static inline void usbStall(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_STALL);}

#endif // USB_DEF_H

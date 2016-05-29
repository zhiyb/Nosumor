#ifndef USB_H
#define USB_H

#include <stdint.h>
#include "stm32f1xx.h"

#define USBRAM		__attribute__((section(".usbram")))
#define USBTABLE	__attribute__((section(".usbtable")))

#define USB_LOCAL_ADDR(addr)	((uint16_t)(((uint32_t)(addr) - (uint32_t)&_susbram) >> 1))
#define USB_SYS_ADDR(addr)	((void *)((uint32_t)&_susbram + ((uint32_t)(addr) << 1)))
#define USB_NUM_BLOCK(bl, size)	((bl) ? (size) / 32 : (size) / 2)
#define USB_RX_COUNT_REG(n)	((((n) > 62) << 15) | (USB_NUM_BLOCK(((n) > 62), (n)) << 10))
#define USB_RX_COUNT_MASK	0x03ff
#define USB_RX_NUM(c)		(((c) >> 10) & 0x1f)
#define USB_RX_BL(c)		((c) & 0x8000)

#define EP_TX	0
#define EP_RX	1

// Endpoint direction
#define EP_IN		0x80
#define EP_OUT		0x00

// Transfer type
#define EP_CONTROL		0x00
#define EP_ISOCHRONOUS	0x01
#define EP_BULK		0x02
#define EP_INTERRUPT	0x03

// Iso mode synchronisation type
#define EP_ISO_NONE	0x00
#define EP_ISO_ASYNC	0x04
#define EP_ISO_ADAPTIVE	0x08
#define EP_ISO_SYNC	0x0c

// Iso mode usage type
#define EP_ISO_DATA	0x00
#define EP_ISO_FEEDBACK	0x10
#define EP_ISO_EXPLICIT	0x20

#define EP_ADDR_MASK	0x0f

#ifdef __cplusplus
extern "C" {
#endif

struct ep_t {
	uint16_t addr;
	uint16_t RESERVED0;
	__IO uint16_t count;
	uint16_t RESERVED1;
};

extern uint32_t _susbram;
extern struct ep_t eptable[8][2];

void initUSB();
uint32_t usbConfigured();

void usbTransfer(uint8_t epid, uint8_t dir, uint8_t buffSize, uint8_t size, const void *ptr);
void usbTransferEmpty(uint16_t epid, uint16_t dir);
void usbHandshake(uint16_t epid, uint16_t dir, uint16_t type);
static inline void usbDisable(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_DIS);}
static inline void usbNAK(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_NAK);}
static inline void usbStall(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_STALL);}
static inline void usbValid(uint16_t epid, uint16_t dir) {usbHandshake(epid, dir, USB_EP_TX_VALID);}

#ifdef __cplusplus
}
#endif

#endif // USB_H

#ifndef USB_H
#define USB_H

#include <stm32f7xx.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_EPIN_CNT	8
#define USB_EPOUT_CNT	8

#define USB_EPIN	0
#define USB_EPOUT	1

struct usb_t;

// Endpoint handlers
typedef struct epin_t {
	void (*init)(struct usb_t *usb);
} epin_t;
typedef struct epout_t {
	void (*init)(struct usb_t *usb);
	void (*recv)(struct usb_t *usb, uint32_t stat);
} epout_t;

// USB port data structure
typedef struct usb_t {
	USB_OTG_GlobalTypeDef *base;
	// USB RAM & FIFO allocation
	struct {
		uint32_t top, max;
		uint32_t fifo;
	};
	// Endpoint allocation
	uint32_t epcnt[2];
	epin_t epin[USB_EPIN_CNT];
	epout_t epout[USB_EPOUT_CNT];
} usb_t;

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base);
int usb_mode(usb_t *usb);	// 0: Device; 1: Host
void usb_init_device(usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_H

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

// Setup packet
typedef union setup_t {
	struct {
		uint8_t bmRequestType;
		uint8_t bRequest;
		union {
			uint16_t wValue;
			struct {
				uint8_t bIndex;
				uint8_t bType;
			};
		};
		uint16_t wIndex;
		uint16_t wLength;
	};
	uint32_t raw[2];
} setup_t;

// Endpoint handlers
typedef struct epin_t {
	void (*init)(struct usb_t *usb, uint32_t ep);
} epin_t;
typedef struct epout_t {
	void (*init)(struct usb_t *usb, uint32_t ep);
	void (*recv)(struct usb_t *usb, uint32_t stat);
} epout_t;

// Descriptor type
typedef struct desc_t {
	uint32_t size;
	void *p;
} desc_t;

typedef struct const_desc_t {
	uint32_t size;
	const void *p;
} cnost_desc_t;

// Interface handlers
typedef struct usb_if_t {
	// Linked list
	struct usb_if_t *next;
	// Interface ID
	uint32_t id;
	// Private data
	void *data;
	// Function handlers
	void (*config)(struct usb_t *usb, void *data);
	void (*enable)(struct usb_t *usb, void *data);
	void (*setup_std)(struct usb_t *usb, void *data,
			  uint32_t ep, setup_t pkt);
	void (*setup_class)(struct usb_t *usb, void *data,
			    uint32_t ep, setup_t pkt);
} usb_if_t;

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
	// Descriptors
	struct {
		desc_t dev, config;
		desc_t *string;
		uint32_t nstring;
	} desc;
	// Interfaces
	usb_if_t *usbif;
} usb_t;

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base);
int usb_mode(usb_t *usb);	// 0: Device; 1: Host
void usb_init_device(usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_H

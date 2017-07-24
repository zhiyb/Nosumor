#ifndef USB_H
#define USB_H

#include <stm32f7xx.h>
#include <stdint.h>
#include "../macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USB_EPIN_CNT	8
#define USB_EPOUT_CNT	8

#define USB_EPIN	0
#define USB_EPOUT	1

struct usb_t;

// Setup packet
typedef union PACKED setup_t {
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
		union {
			uint16_t wIndex;
			struct {
				uint8_t bID;
				uint8_t bEntityID;
			};
		};
		uint16_t wLength;
	};
	uint32_t raw[2];
} setup_t;

// Endpoint handlers
typedef struct epin_t {
	void *data;
	void (*init)(struct usb_t *usb, uint32_t ep);
	void (*halt)(struct usb_t *usb, uint32_t ep, int halt);
	void (*timeout)(struct usb_t *usb, uint32_t ep);
	void (*xfr_cplt)(struct usb_t *usb, uint32_t ep);
} epin_t;
typedef struct epout_t {
	void *data;
	void (*init)(struct usb_t *usb, uint32_t ep);
	void (*halt)(struct usb_t *usb, uint32_t ep, int halt);
	void (*setup_cplt)(struct usb_t *usb, uint32_t ep);
	void (*xfr_cplt)(struct usb_t *usb, uint32_t ep);
} epout_t;

// Descriptor type
typedef struct desc_t {
	uint32_t size;
	void *p;
} desc_t;

typedef struct const_desc_t {
	uint32_t size;
	const void *p;
} const_desc_t;

typedef struct desc_string_list_t {
	struct desc_string_list_t *next;
	uint32_t lang;
	desc_t desc;
} desc_string_list_t;

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
	void (*disable)(struct usb_t *usb, void *data);
	void (*setup_std)(struct usb_t *usb, void *data,
			  uint32_t ep, setup_t pkt);
	void (*setup_class)(struct usb_t *usb, void *data,
			    uint32_t ep, setup_t pkt);
} usb_if_t;

typedef enum {
	USB_Reset = 0, USB_LowSpeed, USB_FullSpeed, USB_HighSpeed,
} usb_speed_t;

typedef enum {
	USB_Self_Powered = 1, USB_Remote_Wakeup = 2,
} usb_status_t;	// bit-fields

// USB port data structure
typedef struct usb_t {
	USB_OTG_GlobalTypeDef *base;
	usb_speed_t speed;
	usb_status_t status;
	// Setup packet buffers
	setup_t setup;
	void *setup_buf;
	// USB RAM & FIFO allocation
	struct {
		uint32_t top, max;
		uint32_t fifo;
	};
	// Endpoint allocation
	uint32_t nepin, nepout;
	epin_t epin[USB_EPIN_CNT];
	epout_t epout[USB_EPOUT_CNT];
	// Descriptors
	struct {
		desc_t dev, dev_qua, config;
		desc_t lang;
		desc_string_list_t **string;
		uint32_t nstring;
	} desc;
	// Interfaces
	usb_if_t *usbif;
} usb_t;

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base);
int usb_mode(usb_t *usb);	// 0: Device; 1: Host
void usb_init_device(usb_t *usb);
void usb_connect(usb_t *usb, int e);

#ifdef __cplusplus
}
#endif

#endif // USB_H

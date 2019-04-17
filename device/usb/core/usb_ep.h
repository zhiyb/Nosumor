#ifndef USB_EP_H
#define USB_EP_H

#include <stdint.h>
#include <defines.h>
#include "usb_macros.h"

typedef struct {
	uint8_t ep_req;		// Preferred endpoint number
	uint8_t resv;		// Reserve this endpoint
	uint8_t ep;		// Actual endpoint number
	uint8_t active;
} usb_ep_data_t;

typedef struct {
	const char *name;
	uint32_t id;
	struct {
		uint32_t type;
		uint32_t pkt_size;
	};
	void (*activate)();
} usb_ep_t;

#define EP_ANY	0x10

// Alloc a preferred endpoint storage
// ep:   Preferred endpoint number (0xnn or EP_ANY)
// dir:  Endpoint direction (IN, OUT)
// = cb: Endpoint event callbacks
#define USB_EP(ep, dir) \
	static usb_ep_data_t CONCAT(_reserve_usb_ep_data_ ## dir ## _, ep) \
		SECTION(".usb_ep_data." #dir "." STRING(ep) ".1") USED = {ep, 1, 0, 0}; \
	static const usb_ep_t CONCAT(_reserve_usb_ep_text_ ## dir ## _, ep) \
		SECTION(".usb_ep_text." #dir "." STRING(ep) ".1") USED
// Endpoint module name
#define USB_EP_MODULE_PREFIX	"_usb_ep_module."
#define USB_EP_MODULE_ID(name)	HASH(USB_EP_MODULE_PREFIX name)
#ifdef DEBUG
#define USB_EP_MODULE(name)	#name, USB_EP_MODULE_ID(#name)
#else
#define USB_EP_MODULE(name)	USB_EP_MODULE_ID(#name)
#endif
// Reserve/unreserve the endpoint
#define USB_EP_RESERVE(ep, dir, v) \
	CONCAT(_reserve_usb_ep_data_ ## dir ## _, ep).resv = v

void usb_ep_active(USB_OTG_GlobalTypeDef *base);

#if 0
#include <stm32f7xx.h>
#include <stdint.h>
#include "usb_desc.h"

typedef struct usb_t usb_t;
typedef struct epin_t epin_t;
typedef struct epout_t epout_t;

void usb_ep_register(usb_t *usb, const epin_t *epin, int *in, const epout_t *epout, int *out);

uint32_t usb_ep_in_max_size(USB_OTG_GlobalTypeDef *usb, int ep);
uint32_t usb_ep_in_transfer(USB_OTG_GlobalTypeDef *usb, int n, const void *p, uint32_t size);
void usb_ep_in_descriptor(USB_OTG_GlobalTypeDef *usb, int ep, desc_t desc, uint32_t zpkt);
void usb_ep_in_stall(USB_OTG_GlobalTypeDef *usb, int ep);
uint32_t usb_ep_in_active(USB_OTG_GlobalTypeDef *usb, int ep);
int usb_ep_in_wait(USB_OTG_GlobalTypeDef *usb, int n);

void usb_ep_out_transfer(USB_OTG_GlobalTypeDef *usb, int n, void *p,
			 uint8_t scnt, uint8_t pcnt, uint32_t size);
void usb_ep_out_ack(USB_OTG_GlobalTypeDef *usb, int n);
void usb_ep_out_stall(USB_OTG_GlobalTypeDef *usb, int n);
void usb_ep_out_wait(USB_OTG_GlobalTypeDef *usb, int n);
#endif

#endif // USB_EP_H

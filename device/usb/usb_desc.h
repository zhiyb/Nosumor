#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include "usb_setup.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;

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

// Operations
void usb_desc_init(usb_t *usb);
desc_t usb_get_descriptor(usb_t *usb, setup_t pkt);

// Descriptors registering
uint32_t usb_desc_add_string(usb_t *usb, uint16_t id, uint16_t lang, const char *str);
uint8_t usb_desc_add_interface(usb_t *usb, uint8_t bAlternateSetting, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface);
void usb_desc_add_interface_association(usb_t *usb, uint8_t bInterfaceCount,
					uint8_t bFunctionClass, uint8_t bFunctionSubClass,
					uint8_t bFunctionProtocol, uint8_t iFunction);
void usb_desc_add_endpoint(usb_t *usb, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval);
void usb_desc_add_endpoint_sync(usb_t *usb, uint8_t bEndpointAddress,
				uint8_t bmAttributes, uint16_t wMaxPacketSize,
				uint8_t bInterval, uint8_t bRefresh, uint8_t bSynchAddress);
void usb_desc_add(usb_t *usb, const void *ptr, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // USB_DESC_H

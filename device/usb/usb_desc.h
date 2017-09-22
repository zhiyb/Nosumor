#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include "usb_setup.h"

// Endpoint direction
#define EP_DIR_Msk	0x80
#define EP_DIR_IN	0x80
#define EP_DIR_OUT	0x00

// Transfer type
#define EP_CONTROL	0x00
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
#define EP_ISO_IMPLICIT	0x20

// Language IDs
#define LANG_ZH_CN	0x0804
#define LANG_EN_US	0x0409
#define LANG_EN_GB	0x0809

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

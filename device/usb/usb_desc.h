#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include <stm32f7xx.h>
#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Endpoint direction
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
#define EP_ISO_EXPLICIT	0x20

desc_t usb_desc_device(usb_t *usb);
desc_t usb_desc_config(usb_t *usb);

void usb_desc_add_interface(usb_t *usb, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface);
void usb_desc_add(usb_t *usb, const void *ptr, uint8_t size);
void usb_desc_add_endpoint(usb_t *usb, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval);

#ifdef __cplusplus
}
#endif

#endif // USB_DESC_H

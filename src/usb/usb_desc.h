#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>

#define REQ_GET_DESCRIPTOR	6
#define REQ_SET_DESCRIPTOR	7

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

struct desc_t {
	const void *data;
	const uint32_t size;
};

struct descriptor_t {
	const struct {
		const struct desc_t *list;
		const uint32_t num;
	} device, config, report;
};

extern const struct descriptor_t descriptors;

#endif // USB_DESC_H

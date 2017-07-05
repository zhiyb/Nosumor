#include <malloc.h>
#include <string.h>
#include "usb_desc.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_setup.h"
#include "usb_macros.h"

#define USB_VID		0x0483
#define USB_PID		0x5750
#define USB_RELEASE	0

/* Device descriptor */

typedef struct PACKED desc_dev_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} desc_dev_t;

static const desc_dev_t desc_dev = {
	18u, SETUP_DESC_TYPE_DEVICE, 0x0200u, 0u, 0u, 0u, 64u,
	USB_VID, USB_PID, USB_RELEASE, 0u, 0u, 0u, 1u
};

desc_t usb_desc_device(usb_t *usb)
{
	if (usb->desc.dev.size)
		return usb->desc.dev;
	desc_dev_t *pd = (desc_dev_t *)malloc(desc_dev.bLength);
	memcpy(pd, &desc_dev, desc_dev.bLength);
	pd->bMaxPacketSize0 = usb_ep0_max_size(usb->base);
	usb->desc.dev.p = pd;
	usb->desc.dev.size = pd->bLength;
	return usb->desc.dev;
}

/* Configration descriptor */

typedef struct PACKED desc_config_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} desc_config_t;

enum {ConfigAttributeSelfPowered = 1u << 6u,
	ConfigAttributeRemoteWakeup = 1u << 5u};

static const desc_config_t desc_config = {
	9u, SETUP_DESC_TYPE_CONFIGURATION, 9u,
	0u, 1u, 0u, 0x80 | ConfigAttributeRemoteWakeup, 250,
};

desc_t usb_desc_config(usb_t *usb)
{
	if (usb->desc.config.size)
		return usb->desc.config;
	desc_config_t *cp = (desc_config_t *)malloc(desc_config.bLength);
	memcpy(cp, &desc_config, desc_config.bLength);
	usb->desc.config.p = cp;
	usb->desc.config.size = cp->bLength;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next) {
		(*ip)->id = cp->bNumInterfaces;
		(*ip)->config(usb, (*ip)->data);
		cp = (desc_config_t *)usb->desc.config.p;
	}
	cp->wTotalLength = usb->desc.config.size;
	return usb->desc.config;
}

/* Interface descriptor */

typedef struct PACKED desc_interface_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} desc_interface_t;

static const desc_interface_t desc_interface = {
	9u, SETUP_DESC_TYPE_INTERFACE, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
};

void usb_desc_add_interface(usb_t *usb, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + desc_interface.bLength);
	desc_interface_t *ip = (desc_interface_t *)(usb->desc.config.p + usb->desc.config.size);
	memcpy(ip, &desc_interface, desc_interface.bLength);
	ip->bNumEndpoints = bNumEndpoints;
	ip->bInterfaceClass = bInterfaceClass;
	ip->bInterfaceSubClass = bInterfaceSubClass;
	ip->bInterfaceProtocol = bInterfaceProtocol;
	ip->iInterface = iInterface;
	usb->desc.config.size += ip->bLength;
	desc_config_t *pc = (desc_config_t *)usb->desc.config.p;
	ip->bInterfaceNumber = pc->bNumInterfaces;
	pc->bNumInterfaces++;
}

/* Endpoint descriptor */

typedef struct PACKED desc_ep_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} desc_ep_t;

static const desc_ep_t desc_ep = {
	7u, SETUP_DESC_TYPE_ENDPOINT, 0u, 0u, 0u, 0u,
};

void usb_desc_add_endpoint(usb_t *usb, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + desc_ep.bLength);
	desc_ep_t *ep = (desc_ep_t *)(usb->desc.config.p + usb->desc.config.size);
	memcpy(ep, &desc_ep, desc_ep.bLength);
	ep->bEndpointAddress = bEndpointAddress;
	ep->bmAttributes = bmAttributes;
	ep->wMaxPacketSize = wMaxPacketSize;
	ep->bInterval = bInterval;
	usb->desc.config.size += ep->bLength;
}

/* Arbitrary descriptor */

void usb_desc_add(usb_t *usb, const void *ptr, uint8_t size)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + size);
	void *p = usb->desc.config.p + usb->desc.config.size;
	memcpy(p, ptr, size);
	usb->desc.config.size += size;
}

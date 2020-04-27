#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include <list.h>

typedef enum {UsbDescDevice, UsbDescConfiguration, UsbDescDeviceQualifier, UsbDescString} usb_desc_type_t;

typedef struct usb_desc_t {
	uint32_t size;
	void *p;
} usb_desc_t;

typedef struct usb_const_desc_t {
	uint32_t size;
	const void *p;
} usb_const_desc_t;

#include <usb/ep0/usb_ep0_setup.h>

void usb_get_descriptor(usb_desc_t *pdesc, usb_setup_t pkt);

// String descriptor

typedef const struct usb_desc_string_t {
	struct usb_desc_string_t *next;
	uint16_t lang;
	const char *p;
} usb_desc_string_t;

// Add a string descriptor
// USB_DESC_STRING(name(optional)) = {next, lang, ptr};
#define USB_DESC_STRING(...)		LIST_ITEM(usb_desc_string, usb_desc_string_t, ## __VA_ARGS__)

// String descriptor list declaration
#define USB_DESC_STRING_LIST()		LIST(usb_desc_string, usb_desc_string_t)
// String descriptor index (ID)
#define USB_DESC_STRING_INDEX(name)	LIST_INDEX(usb_desc_string, name);

// Configuration descriptor append
uint8_t usb_desc_add_interface(usb_desc_t *pdesc, uint8_t bAlternateSetting, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface);
void usb_desc_add_endpoint(usb_desc_t *pdesc, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval);
void usb_desc_add(usb_desc_t *pdesc, const void *p, uint32_t bLength);

typedef void (*const usb_desc_handler_t)(usb_desc_t *pdesc);
#define USB_DESC_CONFIG_HANDLER(func)	LIST_ITEM(usb_desc_config, usb_desc_handler_t) = func

#endif // USB_DESC_H

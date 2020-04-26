#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include <usb/ep0/usb_ep0_setup.h>

typedef enum {UsbDescDevice, UsbDescConfiguration, UsbDescDeviceQualifier, UsbDescString} usb_desc_type_t;

typedef struct usb_desc_t {
	uint32_t size;
	void *p;
} usb_desc_t;

typedef struct usb_const_desc_t {
	uint32_t size;
	const void *p;
} usb_const_desc_t;

typedef struct usb_desc_string_list_t {
	struct usb_desc_string_list_t *next;
	usb_desc_t desc;
	uint32_t lang;
} usb_desc_string_list_t;

void usb_get_descriptor(usb_setup_t pkt, usb_desc_t *desc);

uint32_t usb_desc_add_string(uint16_t id, uint16_t lang, const char *str);

#endif // USB_DESC_H

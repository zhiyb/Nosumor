#ifndef USB_HID_H
#define USB_HID_H

#include <usb/usb_desc.h>

// HID descriptor

typedef struct usb_hid_desc_t {
	uint32_t size;
	uint32_t len;
	void *p;
} usb_hid_desc_t;

// Add a HID report descriptor handler
// USB_HID_DESC(name(optional)) = {next, lang, ptr};
typedef void (*const usb_hid_desc_handler_t)(usb_hid_desc_t *pdesc);
#define USB_HID_DESC_HANDLER(func)	LIST_ITEM(usb_hid_desc, usb_hid_desc_handler_t) = func

#endif // USB_HID_H

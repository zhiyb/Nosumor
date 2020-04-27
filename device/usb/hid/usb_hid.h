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
#define USB_HID_DESC_HANDLER(...)	LIST_ITEM(usb_hid_desc, usb_hid_desc_handler_t, ## __VA_ARGS__)
#define USB_HID_DESC_HANDLER_LIST()	LIST(usb_hid_desc, usb_hid_desc_handler_t)
#define USB_HID_DESC_HANDLER_INDEX(p)	LIST_INDEX(usb_hid_desc, p)
#define USB_HID_DESC_HANDLER_ID(p)	(USB_HID_DESC_HANDLER_INDEX(p) + 1)

// Send report

typedef struct usb_hid_report_t {
	struct usb_hid_report_t * volatile next;
	uint32_t len;
	void *p;
	// Callback on report sent
	void (*cb)(struct usb_hid_report_t *prpt);
} usb_hid_report_t;

void usb_hid_report_enqueue(usb_hid_report_t *prpt);

#endif // USB_HID_H

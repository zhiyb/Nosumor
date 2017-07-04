#include "usb_keyboard.h"
#include "../usb_ram.h"
#include "../usb_desc.h"

typedef struct desc_hid_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;
	uint8_t bClassDescriptorType;
	uint16_t wDescriptorLength;
} desc_hid_t;

static const desc_hid_t desc_hid = {
	9u, 0x21u, 0x0111u, 0x00u, 1u, 0x22u, 0u,
};

static void usb_keyboard_config(usb_t *usb)
{
	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	usb_desc_add_interface(usb, 1u, 3u, 0u, 1u, 0u);
	usb_desc_add(usb, &desc_hid, desc_hid.bLength);
}

void usb_keyboard_init(usb_t *usb)
{
	usb_if_t usbif;
	usbif.config = &usb_keyboard_config;
	usb_interface_alloc(usb, &usbif);
}

#ifndef USB_HID_KEYBOARD_H
#define USB_HID_KEYBOARD_H

#include <stdint.h>
#include <macros.h>

typedef struct PACKED {
	uint8_t id;
	uint8_t modifier;
	uint8_t reserved;
	uint8_t key[6];
} usb_hid_keyboard_report_t;

void usb_hid_keyboard_report_update(usb_hid_keyboard_report_t *p);

#endif // USB_HID_KEYBOARD_H

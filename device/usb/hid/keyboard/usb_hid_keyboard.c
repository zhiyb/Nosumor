#include <malloc.h>
#include <string.h>
#include <debug.h>
#include "usb_hid_keyboard.h"

static const uint8_t desc_report[] = {
	// Keyboard
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x06,		// Usage (Keyboard)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
#if 1	// Modifier keys
	0x75, 0x01,		//   Report size (1)
	0x95, 0x08,		//   Report count (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0xe0,		//   Usage minimum (Left control)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x25, 0x01,		//   Logical maximum (1)
	0x81, 0x02,		//   Input (Data, Var, Abs)
#endif
#if 1	// Reserved
	0x95, 0x01,		//   Report count (1)
	0x75, 0x08,		//   Report size (8)
	0x81, 0x01,		//   Input (Cnst)
#endif
#if 0	// LEDs
	0x95, 0x08,		//   Report count (8)
	0x75, 0x01,		//   Report size (1)
	0x05, 0x08,		//   Usage page (LEDs)
	0x19, 0x01,		//   Usage minimum (Num lock)
	0x29, 0x07,		//   Usage maximum (Shift)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	0x95, 0x01,		//   Report count (1)
	0x75, 0x01,		//   Report size (1)
	0x91, 0x01,		//   Output (Cnst)
#endif
	// Keys
	0x95, 0x06,		//   Report count (6)
	0x75, 0x08,		//   Report size (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0x00,		//   Usage minimum (No event)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xe7, 0x00,	//   Logical maximum (231)
	0x81, 0x00,		//   Input (Data, Ary)
	0xc0,			// End collection
};

#define KEYBOARD_REPORT_SIZE	9u

usb_hid_t *usb_hid_keyboard_init(usb_hid_data_t *p)
{
	usb_hid_t *hid = calloc(1u, sizeof(usb_hid_t) + KEYBOARD_REPORT_SIZE - 1u);
	if (!hid)
		panic();
	hid->hid_data = (usb_hid_data_t *)p;
	hid->size = KEYBOARD_REPORT_SIZE;
	const_desc_t desc = {
		.p = desc_report,
		.size = sizeof(desc_report),
	};
	usb_hid_register(hid, desc);
	return hid;
}

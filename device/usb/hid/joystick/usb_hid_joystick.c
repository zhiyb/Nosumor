#include <malloc.h>
#include <string.h>
#include <debug.h>
#include "usb_hid_joystick.h"

static const uint8_t desc_report[] = {
	// Keyboard
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x04,		// Usage (Joystick)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
	// X, Y, Z axes
	0x05, 0x01,		//   Usage page (Generic desktop)
	0x09, 0x01,		//   Usage (Pointer)
	0xa1, 0x00,		//   Collection (Physical)
	0x09, 0x30,		//     Usage (X)
	0x09, 0x31,		//     Usage (Y)
	0x09, 0x32,		//     Usage (Z)
	0x16, 0x00, 0x80,	//     Logical minimum (-32768)
	0x26, 0xff, 0x7f,	//     Logical maximum (32767)
	0x95, 0x03,		//     Report count (3)
	0x75, 0x10,		//     Report size (16)
	0x81, 0x02,		//     Input (Data, Var, Abs)
	0xc0,			//   End collection
	// Rx, Ry, Rz axes
	0x05, 0x01,		//   Usage page (Generic desktop)
	0x09, 0x01,		//   Usage (Pointer)
	0xa1, 0x00,		//   Collection (Physical)
	0x09, 0x33,		//     Usage (Rx)
	0x09, 0x34,		//     Usage (Ry)
	0x09, 0x35,		//     Usage (Rz)
	0x16, 0x00, 0x80,	//     Logical minimum (-32768)
	0x26, 0xff, 0x7f,	//     Logical maximum (32767)
	0x95, 0x03,		//     Report count (3)
	0x75, 0x10,		//     Report size (16)
	0x81, 0x02,		//     Input (Data, Var, Abs)
	0xc0,			//   End collection
	// Buttons
	0x05, 0x09,		//   Usage page (Button)
	0x19, 0x01,		//   Usage minimum (Button 1)
	0x29, 0x05,		//   Usage maximum (Button 5)
	0x15, 0x00,		//   Logical minimum (0)
	0x25, 0x01,		//   Logical maximum (1)
	0x35, 0x00,		//   Physical minimum (0)
	0x45, 0x01,		//   Physical maximum (1)
	0x95, 0x05,		//   Report count (5)
	0x75, 0x01,		//   Report size (1)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// Reserved
	0x95, 0x01,		//   Report count (1)
	0x75, 0x03,		//   Report size (3)
	0x81, 0x01,		//   Input (Cnst)
	0xc0,			// End collection
};

#define JOYSTICK_REPORT_SIZE	14u

usb_hid_if_t *usb_hid_joystick_init(usb_hid_t *p)
{
	usb_hid_if_t *hid = calloc(1u, sizeof(usb_hid_if_t) + JOYSTICK_REPORT_SIZE - 1u);
	if (!hid)
		panic();
	hid->hid_data = (usb_hid_t *)p;
	hid->size = JOYSTICK_REPORT_SIZE;
	const_desc_t desc = {
		.p = desc_report,
		.size = sizeof(desc_report),
	};
	usb_hid_register(hid, desc);
	return hid;
}

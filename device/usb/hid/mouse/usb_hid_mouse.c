#include <malloc.h>
#include <string.h>
#include <debug.h>
#include "usb_hid_mouse.h"

static const uint8_t desc_report[] = {
	// Keyboard
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x02,		// Usage (Mouse)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
	0xa1, 0x02,		//   Collection (Logical)
	0x09, 0x01,		//     Usage (Pointer)
	0xa1, 0x00,		//     Collection (Physical)
	// Buttons
	0x05, 0x09,		//       Usage page (Button)
	0x19, 0x01,		//       Usage minimum (Button 1)
	0x29, 0x05,		//       Usage maximum (Button 5)
	0x15, 0x00,		//       Logical minimum (0)
	0x25, 0x01,		//       Logical maximum (1)
	0x75, 0x01,		//       Report size (1)
	0x95, 0x05,		//       Report count (5)
	0x81, 0x02,		//       Input (Data, Var, Abs)
	// Padding
	0x75, 0x03,		//       Report size (3)
	0x95, 0x01,		//       Report count (1)
	0x81, 0x01,		//       Input (Cnst)
	// Axes
	0x05, 0x01,		//       Usage page (Generic desktop)
	0x09, 0x30,		//       Usage (X)
	0x09, 0x31,		//       Usage (Y)
	0x15, 0x81,		//       Logical minimum (-127)
	0x25, 0x7f,		//       Logical maximum (127)
	0x75, 0x08,		//       Report size (8)
	0x95, 0x02,		//       Report count (2)
	0x81, 0x06,		//       Input (Data, Var, Rel)
	0xc0,			//     End collection
	0xc0,			//   End collection
	0xc0,			// End collection
};

#define REPORT_SIZE	4u

usb_hid_if_t *usb_hid_mouse_init(usb_hid_t *p)
{
	usb_hid_if_t *hid = calloc(1u, sizeof(usb_hid_if_t) + REPORT_SIZE - 1u);
	if (!hid)
		panic();
	hid->hid_data = (usb_hid_t *)p;
	hid->size = REPORT_SIZE;
	const_desc_t desc = {
		.p = desc_report,
		.size = sizeof(desc_report),
	};
	usb_hid_register(hid, desc);
	return hid;
}

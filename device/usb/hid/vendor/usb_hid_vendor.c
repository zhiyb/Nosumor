#include <malloc.h>
#include <string.h>
#include "usb_hid_vendor.h"

static const uint8_t desc_report[] = {
	// Vendor defined HID
	0x06, 0x39, 0xff,	// Usage page (Vendor defined)
	0x09, 0xff,		// Usage (Vendor usage)
	0xa1, 0x03,		// Collection (Report)
	0x85, 0,		//   Report ID
	// IN type
	0x75, 16,		//   Report size (16)
	0x95, 2,		//   Report count (2)
	0x15, 0x00,		//   Logical minimum (0)
	0x27,			//   Logical maximum
	0xff, 0xff, 0x00, 0x00,	//     (65535)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// IN data
	0x75, 16,		//   Report size (16)
	0x95, 1,		//   Report count (1)
	0x15, 0x00,		//   Logical minimum (0)
	0x27,			//   Logical maximum
	0xff, 0xff, 0x00, 0x00,	//     (65535)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// OUT type
	0x75, 16,		//   Report size (16)
	0x95, 2,		//   Report count (2)
	0x15, 0x00,		//   Logical minimum (0)
	0x27,			//   Logical maximum
	0xff, 0xff, 0x00, 0x00,	//     (65535)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	// OUT data
	0x75, 16,		//   Report size (16)
	0x95, 1,		//   Report count (1)
	0x15, 0x00,		//   Logical minimum (0)
	0x27,			//   Logical maximum
	0xff, 0xff, 0x00, 0x00,	//     (65535)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	0xc0,			// End collection
};

hid_t *usb_hid_vendor_init(void *p)
{
	hid_t *hid = calloc(1u, sizeof(hid_t));
	hid->hid_data = (data_t *)p;
	hid->size = VENDOR_REPORT_IN_SIZE;
	const_desc_t desc = {
		.p = desc_report,
		.size = sizeof(desc_report),
	};
	usb_hid_register(hid, desc);
	return hid;
}

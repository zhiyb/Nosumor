#include <string.h>
#include <usb/hid/usb_hid.h>
#include <usb/usb_macros.h>

// Report descriptor

static const uint8_t desc_report[] = {
	// Vendor defined HID
	0x06, 0x39, 0xff,	// Usage page (Vendor defined)
	0x09, 0xff,		// Usage (Vendor usage)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
	// IN size, cksum, channel
	0x75, 8,		//   Report size (8)
	0x95, 3,		//   Report count (3)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xff, 0x00,	//   Logical maximum (255)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// IN payload
	0x75, 8,		//   Report size (8)
	0x95, 32,		//   Report count (32)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xff, 0x00,	//   Logical maximum (255)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// OUT size, cksum, channel
	0x75, 8,		//   Report size (8)
	0x95, 3,		//   Report count (3)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xff, 0x00,	//   Logical maximum (255)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	// OUT payload
	0x75, 8,		//   Report size (8)
	0x95, 32,		//   Report count (32)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xff, 0x00,	//   Logical maximum (255)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	0xc0,			// End collection
};

static void usb_hid_vendor_desc(usb_hid_desc_t *pdesc);

USB_HID_DESC_HANDLER_LIST();
USB_HID_DESC_HANDLER(usb_hid_vendor_desc_item) = &usb_hid_vendor_desc;

static void usb_hid_vendor_desc(usb_hid_desc_t *pdesc)
{
#if DEBUG >= 5
	if (pdesc->size < pdesc->len + sizeof(desc_report))
		USB_ERROR();
#endif
	uint8_t *p = pdesc->p + pdesc->len;
	memcpy(p, desc_report, sizeof(desc_report));
	// Update report ID
	*(uint8_t *)(p + 8) = USB_HID_DESC_HANDLER_ID(usb_hid_vendor_desc_item);
	pdesc->len += sizeof(desc_report);
}

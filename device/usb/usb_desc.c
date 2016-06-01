#include <stdint.h>
#include "macros.h"
#include "usb.h"
#include "usb_class.h"
#include "usb_desc.h"

// Transfer type
#define EP_CONTROL	0x00
#define EP_ISOCHRONOUS	0x01
#define EP_BULK		0x02
#define EP_INTERRUPT	0x03

// Iso mode synchronisation type
#define EP_ISO_NONE	0x00
#define EP_ISO_ASYNC	0x04
#define EP_ISO_ADAPTIVE	0x08
#define EP_ISO_SYNC	0x0c

// Iso mode usage type
#define EP_ISO_DATA	0x00
#define EP_ISO_FEEDBACK	0x10
#define EP_ISO_EXPLICIT	0x20

static const unsigned char hidReport[] = {
	// Keyboard
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x06,		// Usage (Keyboard)
	0xa1, 0x01,		// Collection (Application)
	0x85, HID_KEYBOARD,	//   Report ID (HID_KEYBOARD)
#if 1
	// Modifier keys
	0x75, 0x01,		//   Report size (1)
	0x95, 0x08,		//   Report count (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0xe0,		//   Usage minimum (Left control)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x25, 0x01,		//   Logical maximum (1)
	0x81, 0x02,		//   Input (Data, Var, Abs)
#endif
#if 0
	0x95, 0x01,		//   Report count (1)
	0x75, 0x08,		//   Report size (8)
	0x81, 0x01,		//   Input (Cnst)
#endif
#if 0
	// LEDs
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
	// Keyboard
	0x95, 0x06,		//   Report count (6)
	0x75, 0x08,		//   Report size (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0x00,		//   Usage minimum (No event)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xe7, 0x00,	//   Logical maximum (231)
	0x81, 0x00,		//   Input (Data, Ary)
	0xc0,			// End collection

	// Mouse
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x02,		// Usage (Mouse)
	0xa1, 0x01,		// Collection (Application)
	0x85, HID_MOUSE,	//   Report ID (HID_MOUSE)
	0x09, 0x01,		//   Usage (Pointer)
	0xa1, 0x00,		//   Collection (Physical)
	// Mouse bottons
	0x95, 0x05,		//     Report count (5)
	0x75, 0x01,		//     Report size (1)
	0x05, 0x09,		//     Usage page (Button)
	0x19, 0x01,		//     Usage minimum (Button 1)
	0x29, 0x05,		//     Usage maximum (Button 5)
	0x15, 0x00,		//     Logical minimum (0)
	0x25, 0x01,		//     Logical maximum (1)
	0x81, 0x02,		//     Input (Data, Var, Abs)
	0x95, 0x01,		//     Report count (1)
	0x75, 0x03,		//     Report size (3)
	0x81, 0x01,		//     Input (Cnst)
	// Mouse XY axes and wheel
	0x95, 0x03,		//     Report count (3)
	0x75, 0x08,		//     Report size (8)
	0x05, 0x01,		//     Usage page (Generic desktop)
	0x09, 0x30,		//     Usage (X)
	0x09, 0x31,		//     Usage (Y)
	0x09, 0x38,		//     Usage (Wheel)
	0x15, 0x81,		//     Logical minimum (-127)
	0x25, 0x7f,		//     Logical maximum (127)
	0x81, 0x06,		//     Input (Data, Var, Rel)
	0xc0,			//   End collection
	0xc0,			// End collection

	// Vendor defined HID
	0x06, 0x39, 0xff,	// Usage page (Vendor defined)
	0x09, 0xff,		// Usage (Vendor usage)
	0xa1, 0x03,		// Collection (Report)
	0x85, HID_VENDOR,	//   Report ID (HID_VENDOR)
	// Type
	0x75, 0x08,		//   Report size (8)
	0x95, 0x01,		//   Report count (1)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xff, 0x00,	//   Logical maximum (255)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	// Data
	0x75, 0x10,		//   Report size (16)
	0x95, 0x02,		//   Report count (2)
	0x15, 0x00,		//   Logical minimum (0)
	0x27,			//   Logical maximum
	0xff, 0xff, 0x00, 0x00,	//     (65535)
	0x09, 0xff,		//   Usage (Vendor usage)
	0x81, 0x02,		//   Input (Data, Var, Abs)
	0xc0,			// End collection
};

//STATIC_ASSERT(sizeof(hidReport) <= 64, "sizeof(hidReport) <= 64");

static const unsigned char device[] = {
	18,			// bLength
	DESC_TYPE_DEVICE,	// bDescriptorType
	0x00,			// bcdUSB		USB specification number
	0x02,
	0,			// bDeviceClass
	0,			// bDeviceSubClass
	0,			// bDeviceProtocol
	64,			// bMaxPacketSize
	L8(USB_VID),		// idVendor
	H8(USB_VID),
	L8(USB_PID),		// idProduct
	H8(USB_PID),
	0x00,			// bcdDevice		Device release number
	0x00,
	0,			// iManufacturer	Manufacturer string
	0,			// iProduct		Product string
	0,			// iSerialNumber	Serial number string
	1,			// bNumConfigurations	Number of possible configurations
};

static const unsigned char config[] = {
	// Configuration descriptor 0
	9,			// bLength
	DESC_TYPE_CONFIG,	// bDescriptorType
	9 * 3 + 7 * 1,		// wTotalLength		Total length of data
	0,
	1,			// bNumInterfaces	Number of interfaces
	1,			// bConfigurationValue	Configuration index
	0,			// iConfiguration	Configuration string
	0xa0,			// bmAttributes		(Remote wakeup)
	250,			// bMaxPower		Max power in 2mA units

	//     Interface descriptor 0
	9,			// bLength
	DESC_TYPE_INTERFACE,	// bDescriptorType
	0,			// bInterfaceNumber	Number of interface
	0,			// bAlternateSetting	Alternative setting
	1,			// bNumEndpoints	Number of endpoints used
	3,			// bInterfaceClass	3: HID class
	0,			// bInterfaceSubClass	1: Boot interface
	2,			// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	0,			// iInterface		Interface string

	//         HID descriptor
	9,			// bLength
	DESC_TYPE_HID,		// bDescriptorType
	0x11,			// bcdHID		HID class specification
	0x01,
	0x00,			// bCountryCode		Hardware target country
	1,			// bNumDescriptors	Number of HID class descriptors
	0x22,			// bDescriptorType	Report descriptor type
	ARRAY_SIZE(hidReport),	// wDescriptorLength	Total length of Report descriptor
	0,

	//         Endpoint descriptor 1
	7,			// bLength
	DESC_TYPE_ENDPOINT,	// bDescriptorType
	EP_IN | 1,		// bEndpointAddress
	EP_INTERRUPT,		// bmAttributes
	EP1_SIZE,		// wMaxPacketSize	Maximum packet size
	0,
	1,			// bInterval		Polling interval
};

static const struct desc_t desc_device[] = {
	{device, sizeof(device)},
};

static const struct desc_t desc_config[] = {
	{config, sizeof(config)},
};

static const struct desc_t desc_report[] = {
	{hidReport, sizeof(hidReport)},
};

const struct descriptor_t descriptors = {
	{desc_device, ARRAY_SIZE(desc_device)},
	{desc_config, ARRAY_SIZE(desc_config)},
	{desc_report, ARRAY_SIZE(desc_report)},
};

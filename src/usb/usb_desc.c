#include <stdint.h>
#include "macros.h"
#include "usb_def.h"
#include "usb_ep0.h"
#include "usb_class.h"
#include "usb_desc.h"

#define USB_VID		0x0483
#define USB_PID		0x5750

static const unsigned char hidReport[] = {
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x06,		// Usage (Keyboard)
	0xa1, 0x01,		// Collection (Application)
	0x85, HID_KEYBOARD,	//   Report ID (HID_KEYBOARD)
	0xc0,			// End collection

	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x02,		// Usage (Mouse)
	0xa1, 0x01,		// Collection (Application)
	0x85, HID_MOUSE,	//   Report ID (HID_MOUSE)
	0x09, 0x01,		//   Usage (Pointer)
	0xa1, 0x00,		//   Collection (Physical)
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

	0x06, 0x39, 0xff,	// Usage page (Vendor defined)
	0x09, 0x39,		// Usage (Vendor usage)
	0xa1, 0x01,		// Collection (Application)
	0x85, HID_VENDOR,	//   Report ID (HID_VENDOR)
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

void usbHIDReport(const void *ptr, uint8_t size)
{
	usbTransfer(1, EP_TX, EP1_SIZE, size, ptr);
}

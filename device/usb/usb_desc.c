#include "usb_desc.h"
#include "usb_ep0.h"
#include "usb_setup.h"

#define USB_VID		0x0483
#define USB_PID		0x5750
#define USB_RELEASE	0

typedef struct desc_dev_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} desc_dev_t;

desc_dev_t desc_dev = {
	18u, SETUP_DESC_TYPE_DEVICE, 0x0200u,
	0u, 0u, 0u,
	64u, USB_VID, USB_PID, USB_RELEASE,
	0u, 0u, 0u, 0u
};

desc_t usb_desc_device(USB_OTG_GlobalTypeDef *usb)
{
	desc_dev.bMaxPacketSize0 = usb_ep0_max_size(usb);
	desc_t desc = {desc_dev.bLength, (void *)&desc_dev};
	return desc;
}

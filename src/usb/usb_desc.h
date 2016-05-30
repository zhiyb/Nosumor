#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>

#define REQ_GET_DESCRIPTOR	6
#define REQ_SET_DESCRIPTOR	7

#define DESC_TYPE_DEVICE	0x01
#define DESC_TYPE_CONFIG	0x02
#define DESC_TYPE_INTERFACE	0x04
#define DESC_TYPE_ENDPOINT	0x05
#define DESC_TYPE_DEV_QUALIFIER	0x06
#define DESC_TYPE_HID		0x21
#define DESC_TYPE_REPORT	0x22
#define DESC_TYPE_PHYSICAL	0x23

#define HID_KEYBOARD	1
#define HID_MOUSE	2
#define HID_VENDOR	3

struct mouse_t {
	uint8_t reportID;
	uint8_t buttons;
	int8_t x, y, wheel;
};

struct desc_t {
	const void *data;
	const uint32_t size;
};

struct descriptor_t {
	const struct {
		const struct desc_t *list;
		const uint32_t num;
	} device, config, report;
};

extern const struct descriptor_t descriptors;

#endif // USB_DESC_H

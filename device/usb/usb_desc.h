#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>

#define USB_VID		0x0483
#define USB_PID		0x5750

#define HID_KEYBOARD	1
#define HID_MOUSE	2

#define REQ_GET_DESCRIPTOR	6
#define REQ_SET_DESCRIPTOR	7

#define DESC_DEVICE		0x01
#define DESC_CONFIG		0x02
#define DESC_STRING		0x03
#define DESC_INTERFACE		0x04
#define DESC_ENDPOINT		0x05
#define DESC_DEV_QUALIFIER	0x06
#define DESC_HID		0x21
#define DESC_REPORT		0x22
#define DESC_PHYSICAL		0x23

#ifdef __cplusplus
extern "C" {
#endif

struct keyboard_t {
	uint8_t reportID;
	uint8_t modifiers;
	//uint8_t reserved;
	uint8_t keycode[6];
};
#define HID_REPORT_KEYBOARD_SIZE	8

struct keyboard_led_t {
	uint8_t reportID;
	uint8_t leds;
};
#define HID_REPORT_KEYBOARD_LED_SIZE	2

struct mouse_t {
	uint8_t reportID;
	uint8_t buttons;
	int8_t x, y, wheel;
};
#define HID_REPORT_MOUSE_SIZE		5

struct vendor_in_t {
	uint32_t timestamp;
	uint16_t status;
};
#define HID_REPORT_VENDOR_IN_SIZE	6

struct desc_t {
	const void *data;
	const uint32_t size;
};

struct descriptor_t {
	const struct {
		const struct desc_t *list;
		const uint32_t num;
	} device, config, report, string;
};

extern const struct descriptor_t descriptors;

#ifdef __cplusplus
}
#endif

#endif // USB_DESC_H

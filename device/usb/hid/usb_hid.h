#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <macros.h>
#include "../usb.h"
#include "../usb_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_hid_data_t usb_hid_data_t;

typedef union ALIGNED {
	struct {
		uint8_t id;
		uint8_t payload[];
	};
	uint8_t raw[1];
} usb_hid_report_t;

typedef struct usb_hid_t {
	// Linked list
	struct usb_hid_t *next;
	// Function handlers
	void (*recv)(struct usb_hid_t *hid, usb_hid_report_t *report, uint32_t size);
	// Data
	usb_hid_data_t *hid_data;
	void *data;
	// Report status for IN transfer
	volatile uint8_t pending;
	uint8_t size;
	usb_hid_report_t report;
} usb_hid_t;

usb_hid_data_t *usb_hid_init(usb_t *usb);
void usb_hid_update(usb_hid_t *hid);

// Register a HID usage
// Return: Report ID
void usb_hid_register(usb_hid_t *hid, const_desc_t desc_report);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_H

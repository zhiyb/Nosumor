#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <macros.h>
#include "../usb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct data_t data_t;

typedef union ALIGNED {
	struct {
		uint8_t id;
		uint8_t payload[];
	};
	uint8_t raw[1];
} report_t;

typedef struct hid_t {
	// Linked list
	struct hid_t *next;
	// Function handlers
	void (*recv)(struct hid_t *hid, report_t *report, uint32_t size);
	// Data
	struct data_t *hid_data;
	void *data;
	// Report status for IN transfer
	uint8_t pending, size;
	report_t report;
} hid_t;

data_t *usb_hid_init(usb_t *usb);
void usb_hid_update(hid_t *hid);

// Register a HID usage
// Return: Report ID
void usb_hid_register(hid_t *hid, const_desc_t desc_report);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_H

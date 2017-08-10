#include <malloc.h>
#include <string.h>
#include <vendor_defs.h>
#include <usb/usb_debug.h>
#include "usb_hid_vendor.h"
#include "../usb_hid.h"

static const uint8_t desc_report[] = {
	// Vendor defined HID
	0x06, 0x39, 0xff,	// Usage page (Vendor defined)
	0x09, 0xff,		// Usage (Vendor usage)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
	// IN size, type
	0x75, 8,		//   Report size (8)
	0x95, 2,		//   Report count (2)
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
	// OUT size, type
	0x75, 8,		//   Report size (8)
	0x95, 2,		//   Report count (2)
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

typedef struct data_t {
	struct data_t *prev, *next;
	struct {
		uint8_t id;
		uint8_t size;
		uint8_t type;
		uint8_t payload[];
	} report;
} data_t;

static void hid_report(hid_t *hid, report_t *report, uint32_t size)
{
	vendor_report_t *rp = (vendor_report_t *)report;
	// Push report to hid buffer at the front
	data_t *dp = malloc(sizeof(data_t) + rp->size);
	memcpy(&dp->report, rp->raw, rp->size);
	dp->next = hid->data;
	dp->prev = 0;
	if (hid->data)
		((data_t *)hid->data)->prev = dp;
	hid->data = dp;
}

hid_t *usb_hid_vendor_init(void *hid_data)
{
	hid_t *hid = calloc(1u, sizeof(hid_t) + VENDOR_REPORT_SIZE - 1u);
	hid->hid_data = (data_t *)hid_data;
	hid->recv = &hid_report;
	hid->size = VENDOR_REPORT_SIZE;
	const_desc_t desc = {
		.p = desc_report,
		.size = sizeof(desc_report),
	};
	usb_hid_register(hid, desc);
	return hid;
}

void usb_hid_vendor_process(hid_t *hid, vendor_func_t func)
{
	if (!hid->data)
		return;
	// Find the earliest report (back of the queue)
	data_t *dp;
	for (dp = (data_t *)hid->data; dp->next; dp = dp->next);
	// Iterate to the front
	data_t *prev;
	for (; dp; dp = prev) {
		prev = dp->prev;
		// Detach node
		__disable_irq();
		if (prev)
			prev->next = 0;
		else
			hid->data = 0;
		__enable_irq();
		// Process report
		func(hid, (vendor_report_t *)&dp->report);
		// Release buffer
		free(dp);
	}
}

void usb_hid_vendor_send(hid_t *hid, vendor_report_t *report)
{
	report->id = hid->report.id;
	while (hid->pending);
	memcpy(&hid->report, report, report->size);
	usb_hid_update(hid);
}

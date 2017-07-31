#ifndef USB_HID_VENDOR_H
#define USB_HID_VENDOR_H

#include <stdint.h>
#include "../usb_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VENDOR_REPORT_IN_SIZE	6u

typedef struct vendor_in_t {
	uint32_t timestamp;
	uint16_t status;
} vendor_in_t;

hid_t *usb_hid_vendor_init(void *hid);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_VENDOR_H

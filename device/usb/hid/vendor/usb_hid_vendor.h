#ifndef USB_HID_VENDOR_H
#define USB_HID_VENDOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hid_t hid_t;
typedef union vendor_report_t vendor_report_t;

typedef void (*vendor_func_t)(hid_t *hid, vendor_report_t *report);

hid_t *usb_hid_vendor_init(void *hid_data);
void usb_hid_vendor_process(hid_t *hid, vendor_func_t func);
void usb_hid_vendor_send(hid_t *hid, vendor_report_t *report);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_VENDOR_H

#ifndef USB_HID_VENDOR_H
#define USB_HID_VENDOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_hid_t usb_hid_t;
typedef struct usb_hid_if_t usb_hid_if_t;
typedef union api_report_t api_report_t;

typedef void (*vendor_func_t)(void *hid, api_report_t *report);

usb_hid_if_t *usb_hid_vendor_init(usb_hid_t *hid_data);
void usb_hid_vendor_process(usb_hid_if_t *hid, vendor_func_t func);
uint8_t usb_hid_vendor_id(const usb_hid_if_t *hid);
void usb_hid_vendor_send(usb_hid_if_t *hid, api_report_t *report);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_VENDOR_H

#ifndef USB_HID_KEYBOARD_H
#define USB_HID_KEYBOARD_H

#include <stdint.h>
#include "../../usb.h"
#include "../usb_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

hid_t *usb_hid_keyboard_init(struct usb_t *usb, void *hid);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_KEYBOARD_H

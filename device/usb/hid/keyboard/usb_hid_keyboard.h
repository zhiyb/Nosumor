#ifndef USB_HID_KEYBOARD_H
#define USB_HID_KEYBOARD_H

#include <stdint.h>
#include "../usb_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

usb_hid_t *usb_hid_keyboard_init(usb_hid_data_t *hid);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_KEYBOARD_H

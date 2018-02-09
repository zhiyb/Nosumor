#ifndef USB_HID_JOYSTICK_H
#define USB_HID_JOYSTICK_H

#include <stdint.h>
#include "../usb_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

usb_hid_if_t *usb_hid_joystick_init(usb_hid_t *hid);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_JOYSTICK_H

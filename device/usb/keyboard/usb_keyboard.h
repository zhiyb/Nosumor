#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usb_t;

void usb_keyboard_init(struct usb_t *usb);
void usb_keyboard_update(uint32_t status);

#ifdef __cplusplus
}
#endif

#endif // USB_KEYBOARD_H

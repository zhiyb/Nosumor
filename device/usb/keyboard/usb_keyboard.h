#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

struct usb_t;

void usb_keyboard_init(struct usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_KEYBOARD_H

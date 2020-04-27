#ifndef USB_EP0_H
#define USB_EP0_H

#include <stdint.h>

void *usb_ep0_in_buf(uint32_t *size);
void usb_ep0_in(uint32_t size);
void usb_ep0_in_null();

#endif // USB_EP0_H

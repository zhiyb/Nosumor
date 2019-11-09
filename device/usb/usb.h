#ifndef USB_H
#define USB_H

#include <stdint.h>

void usb_init();

uint32_t usb_connected();
void usb_connect(uint32_t e);

#endif // USB_H

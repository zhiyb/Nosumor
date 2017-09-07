#ifndef USB_RAM_H
#define USB_RAM_H

#include <stdint.h>

typedef struct usb_t usb_t;
typedef struct usb_if_t usb_if_t;

void usb_ram_reset(usb_t *usb);
uint32_t usb_ram_size(usb_t *usb);
uint32_t usb_ram_alloc(usb_t *usb, uint32_t *size);
uint32_t usb_ram_fifo_alloc(usb_t *usb);
void usb_interface_register(usb_t *usb, const usb_if_t *usbif);

#endif // USB_RAM_H

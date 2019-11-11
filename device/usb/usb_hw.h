#ifndef USB_HW_H
#define USB_HW_H

#include <stdint.h>

uint32_t usb_hw_connected();
void usb_hw_connect(uint32_t e);

void *usb_hw_ram_alloc(uint32_t size);
uint32_t usb_hw_ep_alloc(uint32_t dir, uint32_t type, uint32_t size);
void usb_hw_ep_out(uint32_t epnum, void *p, uint32_t setup, uint32_t pkt, uint32_t size);

#endif // USB_HW_H

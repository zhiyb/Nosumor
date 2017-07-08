#ifndef USB_EP0_H
#define USB_EP0_H

#include <stdint.h>
#include <stm32f7xx.h>
#include "usb.h"

void usb_ep0_register(usb_t *usb);
void usb_ep0_init(usb_t *usb);
void usb_ep0_enum(usb_t *usb, uint32_t speed);
uint32_t usb_ep0_max_size(USB_OTG_GlobalTypeDef *usb);

#endif // USB_EP0_H

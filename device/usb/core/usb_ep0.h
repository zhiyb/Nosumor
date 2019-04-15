#ifndef USB_EP0_H
#define USB_EP0_H

void usb_ep0_reserve();

#if 0
#include <stdint.h>
#include <stm32f7xx.h>

typedef struct usb_t usb_t;

void usb_ep0_register(usb_t *usb);
void usb_ep0_init(usb_t *usb);
void usb_ep0_enum(usb_t *usb, uint32_t speed);
void usb_ep0_process(usb_t *usb);
uint32_t usb_ep0_max_size(USB_OTG_GlobalTypeDef *usb);
#endif

#endif // USB_EP0_H

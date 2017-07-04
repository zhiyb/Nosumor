#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include <stm32f7xx.h>
#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

desc_t usb_desc_device(usb_t *usb);
desc_t usb_desc_config(usb_t *usb);

void usb_desc_add_interface(usb_t *usb, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface);
void usb_desc_add(usb_t *usb, const void *ptr, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // USB_DESC_H

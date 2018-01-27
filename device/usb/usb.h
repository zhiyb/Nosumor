#ifndef USB_H
#define USB_H

#include <stm32f7xx.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base);
int usb_mode(usb_t *usb);	// 0: Device; 1: Host
void usb_init_device(usb_t *usb);
void usb_connect(usb_t *usb, int e);
void usb_process(usb_t *usb);
uint32_t usb_active(usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_H

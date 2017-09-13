#ifndef USB_IRQ_H
#define USB_IRQ_H

#include <stdint.h>

typedef struct usb_t usb_t;

void usb_hs_irq_init(usb_t *usb);
void usb_disable(usb_t *usb);
// ep: epnum | EP_DIR_x
void usb_isoc_check(usb_t *usb, uint8_t ep, int enable);

#endif // USB_IRQ_H

#ifndef USB_IRQ_H
#define USB_IRQ_H

typedef struct usb_t usb_t;

void usb_hs_irq_init(usb_t *usb);
void usb_disable(usb_t *usb);

#endif // USB_IRQ_H

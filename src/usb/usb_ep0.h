#ifndef USB_EP0_H
#define USB_EP0_H

#include "stm32f1xx.h"

#define MAX_EP0_SIZE	64

// 0 = Host to Device
// 1 = Device to Host
#define TYPE_DIRECTION		0x80

#define TYPE_RCPT_MASK		0x1f
#define TYPE_RCPT_DEVICE	0x00
#define TYPE_RCPT_INTERFACE	0x01
#define TYPE_RCPT_ENDPOINT	0x02
#define TYPE_RCPT_OTHER		0x03

extern __IO uint32_t ep0rx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));
extern uint32_t ep0tx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));

void usbInitEP0();
void usbResetEP0();
void usbSetupEP0();

#endif // USB_EP0_H

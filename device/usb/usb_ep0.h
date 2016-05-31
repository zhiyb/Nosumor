#ifndef USB_EP0_H
#define USB_EP0_H

#include "stm32f1xx.h"

// 0 = Host to Device
// 1 = Device to Host
#define TYPE_DIRECTION		0x80

#define TYPE_RCPT_MASK		0x1f
#define TYPE_RCPT_DEVICE	0x00
#define TYPE_RCPT_INTERFACE	0x01
#define TYPE_RCPT_ENDPOINT	0x02
#define TYPE_RCPT_OTHER		0x03

#define EP0_SIZE	64

#ifdef __cplusplus
extern "C" {
#endif

void usbEP0Init();
void usbEP0Reset();
void usbEP0Setup();

#ifdef __cplusplus
}
#endif

#endif // USB_EP0_H

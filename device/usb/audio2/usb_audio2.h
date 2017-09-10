#ifndef USB_AUDIO2_H
#define USB_AUDIO2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;

void usb_audio2_init(usb_t *usb);
uint32_t usb_audio2_feedback_cnt();

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO2_H

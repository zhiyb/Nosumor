#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;

void usb_audio_init(usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO_H

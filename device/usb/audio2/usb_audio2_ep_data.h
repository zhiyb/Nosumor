#ifndef USB_AUDIO2_EP_DATA_H
#define USB_AUDIO2_EP_DATA_H

typedef struct usb_t usb_t;
typedef struct usb_audio_t usb_audio_t;

int usb_audio2_ep_data_register(usb_t *usb);
void usb_audio2_ep_data_halt(usb_t *usb, int ep, int halt);

#endif // USB_AUDIO2_EP_DATA_H

#ifndef USB_MSC_H
#define USB_MSC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;
typedef struct usb_msc_t usb_msc_t;

usb_msc_t *usb_msc_init(usb_t *usb);

#ifdef __cplusplus
}
#endif

#endif // USB_MSC_H

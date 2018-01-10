#ifndef USB_MSC_H
#define USB_MSC_H

#include <logic/scsi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;
typedef struct usb_msc_t usb_msc_t;

usb_msc_t *usb_msc_init(usb_t *usb);
scsi_t *usb_msc_scsi_register(usb_msc_t *msc, const scsi_if_t iface);
void usb_msc_process(usb_t *usb, usb_msc_t *msc);

#ifdef __cplusplus
}
#endif

#endif // USB_MSC_H

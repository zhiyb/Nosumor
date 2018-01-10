#ifndef USB_MSC_H
#define USB_MSC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;
typedef struct usb_msc_t usb_msc_t;
typedef struct scsi_t scsi_t;
typedef struct scsi_handlers_t scsi_handlers_t;

usb_msc_t *usb_msc_init(usb_t *usb);
scsi_t *usb_msc_scsi_register(usb_msc_t *msc, const scsi_handlers_t *handlers);
void usb_msc_process(usb_t *usb, usb_msc_t *msc);

#ifdef __cplusplus
}
#endif

#endif // USB_MSC_H

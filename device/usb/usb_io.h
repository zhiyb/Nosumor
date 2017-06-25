#ifndef USB_IO_H
#define USB_IO_H

#ifdef __cplusplus
extern "C" {
#endif

void usb_init();
int usb_mode();	// 0: Device; 1: Host
void usb_init_device();

#ifdef __cplusplus
}
#endif

#endif // USB_IO_H
